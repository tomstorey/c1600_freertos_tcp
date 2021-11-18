        .title "Periodic Interrupt Timer handler"

#include "FreeRTOSConfig.h"

#define KERNEL_IPL_INV_MASK ((~configKERNEL_INTERRUPT_PRIORITY & 0x7) << 8)
#define API_IPL_INV_MASK ((~configMAX_API_CALL_INTERRUPT_PRIORITY & 0x7) << 8)

        .extern xTaskIncrementTick
        .extern vTaskSwitchContext

        .section .text
        .align 2

        .type PITInterruptHandler, @function
        .global PITInterruptHandler
PITInterruptHandler:
        /* Mask interrupts to the API priority level */
        ori.w   #0x0700, %sr
        eori.w  #API_IPL_INV_MASK, %sr

        /* Save current task context */
        movem.l %d0-%fp, %sp@-
        movea.l (pxCurrentTCB), %a0
        move.l  %sp, %a0@

        /* Set SP to ISR stack */
        /* movea.l (xISRStackTop), %sp */
        
        /* Tick */
        jsr     xTaskIncrementTick

        /* If a task switch is required, do it */
        beq     0f
        jsr     vTaskSwitchContext

        /* Restore context of next task */
0:      movea.l (pxCurrentTCB), %a0
        movea.l %a0@, %sp
        movem.l %sp@+, %d0-%fp

        rte

        .end
