#include <stdint.h>
#include <stdbool.h>
#include "C1600R.h"
#include "uart.h"

void init_pll(void);
void init_ports(void);
void init_memory_params(void);
void init_bss_and_data(void);
void init_zeroise_pram(void);
void init_others(void);

void
hardware_init_hook(void)
{
    /* Initialise all the everythings */
    init_pll();
    init_ports();
    init_memory_params();
    init_bss_and_data();
    init_zeroise_pram();
    init_others();

    init_console_uart();
}

void
init_pll(void)
{
    /*
     * Configure the PLL for operational frequency
     * 
     * Requires chip selects to be configured first so that the SYSOPT register
     * can be read to determine CPU speed strap.
     * 
     * The MODCK1-0 configuration on the 1600R is binary 10, this enables the
     * PLL as well as dividing the XTAL frequency by 128 by default on reset.
     * Thus, only the multiplication factor needs to be changed. The prescaler
     * can be disabled, refer to the MC68360 UM for details.
     */
    if (SYSOPTbits.SPEED == 0) {
        PLLCRbits.MF = (FCY_33MHZ / (FCY_XTAL / 128));
    } else {
        PLLCRbits.MF = (FCY_25MHZ / (FCY_XTAL / 128));
    }
}

void
init_ports(void)
{
    /*
     * Default configuration for ports - all ports start as inputs
     */
    PAPAR = 0;
    PBPAR = 0;
    PCPAR = 0;

    PADIR = 0;
    PBDIR = 0;
    PCDIR = 0;

    PAODR = 0;
    PBODR = 0;

    PADAT = 0;
    PBDAT = 0;
    PCDAT = 0;
}

void
init_memory_params(void)
{
    /*
     * Configure memory parameters such as DRAM refreshing, and chip selects.
     *
     * This must be done before most other things, for example, configuring the
     * PLL, as the external peripherals must be accessible to read the CPU
     * speed strap.
     */
    extern uint32_t __ram_end;

    /* CS0/ to map boot ROMs at 0x04000000-0403FFFF (256KB) */
    BR0 = 0x04000003;
    OR0 = 0x5FFC0002;

    /* CS3/ to map external peripherals at 0x0D000000-0D0FFFFF (1MB) */
    BR3 = 0x0D000001;
    OR3 = 0xFFF00006;

    /* Configure DRAM memory interface parameters */
    PEPAR = 0x07C0;
    GMR = 0x07840180;

    /* CS5/ and CS6/ to map on-board RAM at 0x02000000-027FFFFF (8MB) */
    BR5 = 0x02000001;
    OR5 = 0x1FC00001;
    BR6 = 0x02400001;
    OR6 = 0x1FC00001;

    /* CS7/ to map NVRAM at 0x0E000000-0E007FFF (32KB) */
    OR7 = 0x6FFF8004;
    BR7 = 0x0E000001;

    /* Relocate the stack out of the internal DPRAM and into on-board RAM */
    asm volatile(
        /* Setup some frequently used values */
        "    move.l  %[old_stack], %%d1     \n\t"
        "    move.l  %[new_stack], %%d2     \n\t"
        "    move.l  %[frame_term], %%d3    \n\t"

        /* Figure out how many words to copy from old stack to new stack */
        "    move.l  %%d1, %%d0         \n\t"
        "    sub.l   %%sp, %%d0         \n\t"
        "    lsr.l   #1, %%d0           \n\t" /* D0 = words to copy */
        "    movea.l %%d1, %%a5         \n\t" /* A5 = top of old stack */
        "    movea.l %%d2, %%sp         \n\t" /* SP = top of new stack */

        /* Copy old stack to new stack */
        "0:  move.w  %%a5@-, %%sp@-     \n\t"
        "    subq.l  #1, %%d0           \n\t"
        "    bne     0b                 \n\t"

        /* Fix up frame pointer if not terminal value */
        "    cmp.l   %%fp, %%d3         \n\t"
        "    beq     2f                 \n\t"

        "    move.l  %%d1, %%d0         \n\t"
        "    sub.l   %%fp, %%d0         \n\t" /* D0 = total frame size */
        "    movea.l %%d2, %%fp         \n\t"
        "    suba.l  %%d0, %%fp         \n\t" /* FP fixed up */

        /* If value at %fp@ is terminal value, only one frame is linked so
         * skip fixup */
        "    cmp.l   %%fp@, %%d3        \n\t"
        "    beq     2f                 \n\t"

        /* Loop through stack frames and fix up links until terminal value */
        "    movea.l %%fp, %%a4         \n\t" /* Pointer to next link */

        "1:  move.l  %%d1, %%d0         \n\t"
        "    sub.l   %%a4@, %%d0        \n\t" /* D0 = offset of link */
        "    movea.l %%d2, %%a5         \n\t" /* New stack into A5 */
        "    suba.l  %%d0, %%a5         \n\t" /* A5 = new address of link */
        "    move.l  %%a5, %%a4@        \n\t" /* Write new link to stack */
        "    movea.l %%a5, %%a4         \n\t" /* Setup pointer to next link */
        "    cmp.l   %%a4@, %%d3        \n\t" /* Terminal value reached? */
        "    bne     1b                 \n\t"

        "2:                             \n\t"
        :
        :[old_stack]"i"(MODULE_BASE + 0x400), [new_stack]"i"(&__ram_end),
         [frame_term]"i"(FRAME_TERM)
        :"%d0", "%d1", "%d2", "%d3", "%a4", "%a5"
    );
}

void
init_bss_and_data(void)
{
    /*
     * Zeroise the BSS area and copy initialised data to RAM
     * 
     * Requires chip selects to be configured first so that the RAM banks are
     * accessible.
     */
    extern uint32_t _bss_start;
    extern uint32_t _bss_end;
    extern uint32_t _rodata_end;
    extern uint32_t _data_start;
    extern uint32_t _data_end;
    uint32_t *src;
    uint32_t *dst;

    /* BSS */
    for (dst = (uint32_t *)&_bss_start; dst < (uint32_t *)&_bss_end; dst++) {
        *dst = 0;
    }

    /* Initialised data */
    for (src = (uint32_t *)&_rodata_end, dst = (uint32_t *)&_data_start;
         dst < (uint32_t *)&_data_end;
         src++, dst++) {
        *dst = *src;
    }
}

void
init_zeroise_pram(void)
{
    /*
     * Zeroise the User Data area of the DPRAM.
     *
     * Mask rev A and B have 1536 bytes of DPRAM available for BDs, clear this
     * area only. Mask revs C+ have 1792 bytes, but a method to detect the mask
     * version is unknown at present, so assume 1536 bytes.
     */
    uint16_t *dst = (void *)DPRBASE;
    uint16_t size = 1536 / 2;

    for (; size > 0; size--, dst++) {
        *dst = 0;
    }
}

void
init_others(void)
{
    /*
     * Initialise other registers
     */
    SWIV = 0x41;
    PITRbits.SWP = 0;
    SYPCR = 0x97;               /* Enables bus monitor and SWT for IRQ */


    SDCR = 0x0740;              /* Recommended by User Manual */

    CICR = 0x00270001;          /* SCC priorities and spreading */
    CICRbits.IRL = 2;           /* CPM IRQ level */
    CICRbits.HP = 0b00100;      /* Highest priority interrupt is SMC1 */
    CICRbits.VBA = 0b111;       /* Upper 3 bits of CPM interrupt vector */
    
    CR = 0x8001;                /* Reset the CP */

    /* Disable and clear any CP interrupts that might be present */
    CIMR = 0;
    CIPR = 0xFFFFFFFF;
    CISR = 0xFFFFFFFF;

    /* Enable interrupts from the WIC slot */
    INTSRC = 0;                 /* Write anything to clear source bits */
    INTCON = 0xFE;
}
