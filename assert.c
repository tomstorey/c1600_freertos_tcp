#include <stdbool.h>
#include <stdio.h>
#include "uart.h"

void __attribute__((noreturn))
__assert_fail(const char* expr, const char* file, unsigned int line, const char* function)
{
    /* Mask all interrupts */
    asm volatile("ori.w #0x0700, %sr");

    printf("Assertion failed: %s (%s: %s: %u)\r\n", expr, file, function, line);

    while (true);
}
