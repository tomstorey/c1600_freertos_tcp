#ifndef FREERTOS_IP_CONFIG_H
#define	FREERTOS_IP_CONFIG_H

/* FreeRTOSIPConfig.h for use with QUICC Ethernet controller in Cisco 1600R */

#define ipconfigNETWORK_MTU 1514

#define ipconfigUSE_DHCP 0

/* TODO: does the QUICC automatically pad packets to minimum size? */
#define ipconfigETHERNET_MINIMUM_PACKET_BYTES 60

/* TODO: describe ring size settings */
#define ipconfigQUICC_RX_RING_SZ 16
#define ipconfigQUICC_TX_RING_SZ 4
#define ipconfigQUICC_BUFFERS_EXTRA 16

#define ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS (ipconfigQUICC_RX_RING_SZ + ipconfigQUICC_TX_RING_SZ + ipconfigQUICC_BUFFERS_EXTRA)
#define ipconfigEVENT_QUEUE_LENGTH (ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS + 16)

/* IP task properties */
#define ipconfigIP_TASK_PRIORITY (configMAX_PRIORITIES - 1)
#define ipconfigIP_TASK_STACK_SIZE_WORDS configMINIMAL_STACK_SIZE

/* M68k is big endian */
#define ipconfigBYTE_ORDER pdFREERTOS_BIG_ENDIAN

/* The LANCE takes care of checksumming the ethernet frame, but IP checksums
 * need to be calculated in software */
#define ipconfigDRIVER_INCLUDED_TX_IP_CHECKSUM 0
#define ipconfigDRIVER_INCLUDED_RX_IP_CHECKSUM 0

/* Use no-copy method of buffer management */
#define ipconfigZERO_COPY_TX_DRIVER 1
#define ipconfigZERO_COPY_RX_DRIVER 1

#endif	/* FREERTOS_IP_CONFIG_H */
