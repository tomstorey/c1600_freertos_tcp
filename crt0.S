        .title "crt0.S for m68k bare metal - standalone"

#include "C1600R.h"

/* Uncomment the following if you dont want the EVT to be automatically copied
 * to RAM address 0 when the ROM has been remapped away from address 0 */
/* #define NO_EVT_COPY */

#define USEVBR (defined(mc68010) || defined(mc68020) || defined(mc68030) || \
                defined(mc68040) || defined(mc68060) || defined(mc68332))

        /* A bunch of variables supplied by the linker */
        .extern _rom_base
        .weak hardware_init_hook
        .weak software_init_hook
        .extern main

        .section .text
        .align 2

        .type _start, @function
        .globl _start
_start:
        reset                           /* Reset all external peripherals */

        move.w  #0x2700, %sr            /* Mask interrupts, supervisor mode */

        movea.l #MODULE_BASE+0x400, %sp /* Init the SSP to internal DPRAM since
                                         * DRAM is not available yet */
        movea.l #0, %a0                 /* Zeroise USP */
        move.l  %a0, %usp

        movea.l #FRAME_TERM, %fp        /* Configure terminal value for FP for
                                         * use in stack relocation */

        /* Source/Destination function code registers to supervisor space */
        moveq   #7, %d0
        movec   %d0, %sfc
        movec   %d0, %dfc

        /* Configure MBAR to point to address 0x0FF00000 - stack now useable */
        move.l  #MODULE_BASE+1, %d0
        moves.l %d0, (0x3FF00)

        /* Zeroise internal dual-port RAM that will be used for stack */
        move.l  #0x3FF, %d0
        clr.l   %d1
        movea.l #MODULE_BASE, %a0

0:      move.l  %d1, %a0@+
        dbf     %d0, 0b

        /* Make sure Breakpoint Control Register is cleared */
        clr.l   BKCR                    /* BKCR */

        /* If using a CPU with a VBR, point it to the beginning of ROM */
        movea.l #_rom_base, %a0
        movec   %a0, %vbr

        /* Execute hardware init hook if present */
        move.l  #hardware_init_hook, %d0
        beq     1f
        movea.l %d0, %a0
        jsr     (%a0)

        /* Execute software init hook if present */
1:      move.l  #software_init_hook, %d0
        beq     2f
        movea.l %d0, %a0
        jsr     (%a0)

        /* Jump to main() */
2:      jmp     main

        /* In a bare metal environment, main() should not return as there is
         * nowhere to resume execution from. Behaviour is undefined if this
         * happens! */

/*
 * __DefaultInterrupt handles all interrupt and exception vectors that have not
 * been overridden by the programmer.
 *
 * Unless handled more specifically, all exceptions and interrupts are masked
 * and the CPU stops executing.
 */
        .type __DefaultInterrupt, @function
        .weak __DefaultInterrupt
        .globl __DefaultInterrupt
__DefaultInterrupt:
        ori.w   #0x0700, %sr
        move.l  #50000, %d0

        move.b  #0, (LEDCR)

0:      eori.b  #0xFF, (LEDCR)
        move.l  %d0, %d1

1:      dbf     %d1, 1b

        bra     0b

        stop    #0x2700
        bra     .

        .end
