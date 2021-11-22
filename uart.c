#include <stdint.h>
#include <stddef.h>
#include "C1600R.h"

#define CONSOLE_BAUD 57600      /* Baud rate for the console port */
#define MAX_MRBLR 65535         /* The maximum number of bytes to be received
                                 * at a time by a single BD */
#define MAX_TX_LEN 256          /* Maximum bytes to transmit in one go */
#define RX_BUFFER_SZ 256        /* Number of bytes to allocate to the RX buf */
#define TX_BUFFER_SZ 32768      /* Number of bytes to allocate to the TX buf */

/* Store buffer descriptors in DPRAM - two rx BDs are provided for chaining
 * the reception of large amounts of data. Each BD is 8 bytes in size. Store
 * at the very top of User Data area of DPRAM as ethernet BDs will grow up. */
static volatile __SMCRxBufferDescriptor_t *rxbd = (void *)(DPRBASE+0x5E0);
static volatile __SMCTxBufferDescriptor_t *txbd = (void *)(DPRBASE+0x5F0);

/* Some working buffers for the UART */
static volatile uint8_t rx_buffer[RX_BUFFER_SZ] = {0};
static volatile uint8_t tx_buffer[TX_BUFFER_SZ] = {0};

/* Head and tail pointers to keep track of where in the TX buffer data has been
 * written up to and transmitted up to */
//static volatile uint8_t *tx_head = &tx_buffer[0];
//static volatile uint8_t *tx_tail = &tx_buffer[0];
static volatile uint32_t tx_head = 0;
static volatile uint32_t tx_tail = 0;

void
init_console_uart(void)
{
    /*
     * Initialise SMC1 - console port
     */

    /* Configure required port pins */
    PBPARbits.DD6 = 1;          /* PB6 is console tx, SMTXD1 */
    PBPARbits.DD7 = 1;          /* PB7 is console rx, SMRXD1 */

    PBDIRbits.DR8 = 1;          /* PB8 is output, console DTR (sw control)
                                 * PB9 is input, console DST (sw control) */

    /* Configure baud rate generator
     *
     * The Clock Divider can be calculated as such:
     *
     * system frequency / (baud * 16); e.g.
     * 33281250 / (9600 * 16)
     *
     * This gives a CD of 216.67 which could be rounded to either 216 or 217
     * with approx +/- 30bps error (9629 or 9585 respectively).
     *
     * Error rate as a percentage can be calculated as, e.g.:
     *
     * ((9585 - 9600) / 9600) * 1000 = -1.56%
     * ((9629 - 9600) / 9600) * 1000 = +3.02%
     *
     * An absolute error rate of less than 5% is acceptable. */
    BRGC2 = 1 << _BRGC2_RST_POSITION; /* Reset the BRG */

    if (SYSOPTbits.SPEED == 0) {
        /* Configure clock divider to produce the desired baud rate at 33MHz,
         * and enable the BRG */
        BRGC2bits.CD = (FCY_33MHZ / (CONSOLE_BAUD * 16));
    } else {
        /* Configure clock divider to produce the desired baud rate at 25MHz,
         * and enable the BRG */
        BRGC2bits.CD = (FCY_25MHZ / (CONSOLE_BAUD * 16));
    }

    BRGC2bits.ATB = 0;
    BRGC2bits.EN = 1;

    SIMODEbits.SMC1 = 0;        /* Connect BRG2 to SMC1 */
    SIMODEbits.SMC1CS = 1;

    /* Set buffer descriptor pointers */
    SMC1RBASE = (uint16_t)((uint32_t)rxbd);
    SMC1TBASE = (uint16_t)((uint32_t)txbd);

    /* Setup function codes for SMC SDMA channels */
    SMC1RFCRbits.MOT = 1;       /* Motorola byte ordering */
    SMC1RFCRbits.FC = 8;        /* Function code */
    SMC1TFCRbits.MOT = 1;
    SMC1TFCRbits.FC = 8;
    
    SMC1MRBLR = 1;              /* Maximum receive buffer length */

    SMC1MAX_IDL = 0;            /* Disable idle function */

    /* Configure the tx and rx buffer descriptors */
    txbd->SRC = NULL;
    txbd->LEN = 0;
    txbd->flags.W = 1;          /* This is the last BD in the chain */

    rxbd[0].DST = (uint8_t *)&rx_buffer;
    rxbd[0].flags.W = 1;         /* This is the last BD in the chain for now */
    rxbd[0].flags.I = 1;         /* Interrupt on closure */
    rxbd[0].flags.E = 1;         /* BD is empty - CP owns this BD now */

    rxbd[1].DST = (uint8_t *)NULL;
    rxbd[1].flags.W = 1;
    rxbd[1].flags.I = 1;

    /* Configure SMC1 for UART, 8 bits, no parity */
    SMCMR1 = 0;
    SMCMR1bits.CLEN = 9;        /* 8 data bits */
    SMCMR1bits.SM = 2;          /* UART mode */

    /* Initialise params */
    CRbits.OPCODE = 0;          /* Init tx/rx params */
    CRbits.CHNUM = 9;           /* Ch 9 = SMC1 */
    CRbits.FLG = 1;             /* Do it */

    /* Configure interrupts for SMC1 */
    /* TODO: reconfigure interrupts once debugging is complete */
    SMCE1 = 0xFF;               /* Clear all event bits */
    SMCM1bits.RX = 0;           /* ?able rx interrupts */
    SMCM1bits.TX = 1;           /* ?able tx interrupts */
    CIMRbits.SMC1 = 0;          /* ?able interrupts for SMC1 */
    SMCMR1bits.TEN = 1;         /* ?able transmitter */
    SMCMR1bits.REN = 0;         /* ?able receiver */
    PBDATbits.D8 = 0;           /* DTR to idle state */
}

void
putchar_(char ch)
{
    /* Add the byte to the TX buffer and increment head pointer */
    tx_buffer[tx_head++] = ch;

    /* Wrap head pointer if it extends beyond the allocated buffer */
    if (tx_head >= TX_BUFFER_SZ) {
        tx_head = 0;
    }
}

void
uart_flush(void)
{
    uint32_t tx_len = 0;

    __SMCEbits_t events = { .u8 = SMCE1 };
    SMCE1 = events.u8;

    if (txbd->flags.R) {
        /* BD is busy */
        return;
    } else {
        /* BD is not busy, add the length of the transmitted data to the tail
         * pointer */
        tx_tail += txbd->LEN;
        txbd->LEN = 0;

        if (tx_tail >= TX_BUFFER_SZ) {
            tx_tail = 0;
        }
    }

    if (tx_tail == tx_head) {
        /* Nothing to transmit */
        return;
    }

    if (tx_tail < tx_head) {
        /* Tail is behind the head - head has not wrapped */
        tx_len = tx_head - tx_tail;
    } else {
        /* Tail is ahead of the head - head has wrapped */
        tx_len = TX_BUFFER_SZ - tx_tail;
    }

    if (tx_len > MAX_TX_LEN) {
        /* Truncate length */
        tx_len = MAX_TX_LEN;
    }

    /* Send the data */
    txbd->SRC = (uint8_t *)&tx_buffer[tx_tail];
    txbd->LEN = tx_len;
    txbd->flags.u16 = 0xA000;
}

void __attribute__((interrupt))
SMC1InterruptHandler(void)
{
    /* Each time a TX interrupt occurrs on SMC1, work out if there is more
     * data that can be sent over the console port */
//        uint32_t tx_len;

    /* Get pended interrupts */
    const __SMCEbits_t events = { .u8 = SMCE1 };

//    if (events.TX) {
//        /* Increment the tail pointer by the number of bytes that just finished
//         * sending */
//        tx_tail += txbd->LEN;
//
//        /* Wrap tx_tail if it extends beyond the allocated buffer */
//        if (tx_tail >= &tx_buffer[TX_BUFFER_SZ]) {
//            tx_tail = &tx_buffer[0];
//        }
//
//        /* Is there anything more that needs sending? */
//        if (tx_head != tx_tail) {
//            if (tx_tail < tx_head) {
//                /* Head is further along in the buffer, but has not wrapped */
//                tx_len = (uint32_t)(tx_head - tx_tail);
//            } else {
//                /* Tail is further along, head has probably wrapped */
//                tx_len = (uint32_t)(&tx_buffer[TX_BUFFER_SZ] - tx_tail);
//            }
//
//            /* Truncate length if exceeding MAX_TX_LEN */
//            if (tx_len > MAX_TX_LEN) {
//                tx_len = MAX_TX_LEN;
//            }
//
//            /* Send more data */
//            txbd->SRC = (uint8_t *)tx_tail;
//            txbd->LEN = tx_len;
//            txbd->flags.u16 = 0xB000;
//        }
//    }

    /* Clear event/interrupt flags after they have been handled */
    SMCE1 = events.u8;
    CISR = 1 << _CISR_SMC1_POSITION;
}

void __attribute__((interrupt))
SoftwareWatchdogInterruptHandler(void)
{
    /* The SWT runs at IRQ7, so will continue to run even if an exception
     * handler is executed. Call uart_flush() to transmit any queued data out of
     * the console port */
    uart_flush();
}
