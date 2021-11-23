#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "C1600R.h"
#include "FreeRTOS-Kernel/include/FreeRTOS.h"
#include "FreeRTOS-Kernel/include/task.h"
#include "FreeRTOS-Plus-TCP/include/FreeRTOS_IP.h"
#include "FreeRTOS-Plus-TCP/portable/NetworkInterface/C1600-QUICC/NetworkInterface.h"

static TaskHandle_t task_handle1 = NULL;
static TaskHandle_t task_handle2 = NULL;
static TaskHandle_t task_handle3 = NULL;
static TaskHandle_t task_handle4 = NULL;
static TaskHandle_t task_handle5 = NULL;


void
vApplicationSetupInterrupts(void)
{
    /* Configure a timer to generate FreeRTOS tick interrupt */
    PICRbits.PIRQL = 1;         /* IRQ 1 */
    PICRbits.PIV = 0x40;        /* Vector 0x40 */
    PITRbits.CTR = 0x4E;        /* Approx 10ms */

    portENABLE_INTERRUPTS();
}


int
dbgprintf(const char * format, ...)
{
    int result;
    va_list args;

    va_start(args, format);
    result = vprintf(format, args);
    va_end(args);
    printf("\r");

    return result;
}


static void
task1(void *param)
{
    while (true) {
        LEDCRbits.OK ^= 1;

        vTaskDelay(15);
    }
}

static void
task2(void *param)
{
    while (true) {
        LEDCRbits.LED1 ^= 1;

        vTaskDelay(26);
    }
}

static void
task3(void *param)
{
    while (true) {
        LEDCRbits.LED2 ^= 1;

        vTaskDelay(27);
    }
}

static void
task4(void *param)
{
    while (true) {
        LEDCRbits.LED3 ^= 1;

        vTaskDelay(28);
    }
}

static void
task5(void *param)
{
    while (true) {
        LEDCRbits.LED4 ^= 1;

        vTaskDelay(29);
    }
}


uint8_t *
read_mac_from_cisco_cookie(void)
{
    uint16_t cksum = 0;
    uint16_t *data = (uint16_t *)0x0E000000;
    uint8_t ctr;

    if (*data != 0x8D74) {
        /* Magic number is not valid */
        return NULL;
    }

    /* Calculate checksum of cookie to ensure it is valid */
    for (ctr = 17; ctr > 0; ctr--) {
        cksum += *data++;
    }

    if (cksum != *data) {
        /* Checksum invalid */
        return NULL;
    }

    /* Return pointer to MAC address within cookie */
    return (uint8_t *)0x0E000004;
}


int
main(void)
{
    printf("RISC revision: %04X\r\n", RISCREVNUM);

    /* Create some tasks */
    xTaskCreate(task1, "Task 1", configMINIMAL_STACK_SIZE, NULL, 1, &task_handle1);
    xTaskCreate(task2, "Task 2", configMINIMAL_STACK_SIZE, NULL, 2, &task_handle2);
    xTaskCreate(task3, "Task 3", configMINIMAL_STACK_SIZE, NULL, 3, &task_handle3);
    xTaskCreate(task4, "Task 4", configMINIMAL_STACK_SIZE, NULL, 1, &task_handle4);
    xTaskCreate(task5, "Task 5", configMINIMAL_STACK_SIZE, NULL, 2, &task_handle5);

    /* IP stack configuration */
    static uint8_t ucIPAddress[4] = {0xAB, 0xCD, 0xEF, 200};
    static uint8_t ucNetMask[4] = {255, 255, 255, 0};
    static uint8_t ucGatewayAddress[4] = {0xAB, 0xCD, 0xEF, 1};
    static uint8_t ucDNSServerAddress[4] = {8, 8, 8, 8};
    static uint8_t *ucMACAddress = NULL;
    
    ucMACAddress = read_mac_from_cisco_cookie();

    printf("Board MAC: %04X.%04X.%04X\r\n", *(uint16_t *)ucMACAddress,
                                            *(uint16_t *)(ucMACAddress+2),
                                            *(uint16_t *)(ucMACAddress+4));
    printf("Board IP: %d.%d.%d.%d\r\n", ucIPAddress[0], ucIPAddress[1],
                                        ucIPAddress[2], ucIPAddress[3]);

    if (FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress,
                        ucDNSServerAddress, ucMACAddress) != pdTRUE) {
        /* IP stack init failed */
        printf("FreeRTOS_IPInit() failed\r\n");

        while (true);
    }

    /* Start the scheduler */
    while (true) {
        printf("vTaskStartScheduler()\r\n");

        vTaskStartScheduler();
    }
    
    // /* Unmask interrupts */
    // asm volatile ("andi.w #0xF8FF, %sr");

    // uint32_t ctr;

    // while (true) {
    //     for (ctr = 100000; ctr > 0; ctr--);

    //     LEDCRbits.REAROK ^= 1;
    // }

    return 0;
}


BaseType_t
xApplicationGetRandomNumber( uint32_t * pulNumber )
{
    /* TODO: This definitely needs to be more random */
    static uint32_t rand = 0;

    *pulNumber = rand++;

    return pdTRUE;
}

/* Borrowed from MSP432 port */
uint32_t ulApplicationGetNextSequenceNumber( uint32_t ulSourceAddress,
                                             uint16_t usSourcePort,
                                             uint32_t ulDestinationAddress,
                                             uint16_t usDestinationPort )
{
    uint32_t pulNumber = 0;

    xApplicationGetRandomNumber( &pulNumber );

    return pulNumber;
}
