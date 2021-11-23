        .title "Periodic Interrupt Timer Interrupt Handler"

#include "FreeRTOSConfig.h"

#define API_IPL (0x2000 + (configMAX_API_CALL_INTERRUPT_PRIORITY << 8))

        .extern xTaskIncrementTick
        .extern vTaskSwitchContext

        .section .text
        .align 2

        .type PITInterruptHandler, @function
        .globl PITInterruptHandler
PITInterruptHandler:
        /* Mask interrupts to the API priority level */
        move.w  #API_IPL, %sr

        /* Save current task context */
        movem.l %d0-%fp, %sp@-
        movea.l (pxCurrentTCB), %a0
        move.l  %sp, %a0@

        /* Set SP to ISR stack */
        /* Before uncommenting this line across multiple ISRs, some kind of
         * synchronisation is required to ensure that the ISR stack is only
         * configured once for nested interrupts to avoid stack corruption */
        /* movea.l (xISRStackTop), %sp */
        
        /* Tick */
        jsr     xTaskIncrementTick

        /* If a task switch is required, do it */
        move.l  %d0, %d0
        beq     0f
        jsr     vTaskSwitchContext

        /* Restore context of next task */
0:      movea.l (pxCurrentTCB), %a0
        movea.l %a0@, %sp
        movem.l %sp@+, %d0-%fp

        rte

        .end
