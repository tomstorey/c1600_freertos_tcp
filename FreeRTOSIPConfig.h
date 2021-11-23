#ifndef FREERTOS_IP_CONFIG_H
#define	FREERTOS_IP_CONFIG_H

/* FreeRTOSIPConfig.h for use with QUICC Ethernet controller in Cisco 1600R */

/* Enable sanity checks */
#define ipconfigTCP_IP_SANITY 1

extern int dbgprintf(const char * format, ...);

#define ipconfigHAS_DEBUG_PRINTF 0
#if ( ipconfigHAS_DEBUG_PRINTF == 1 )
#define FreeRTOS_debug_printf( X )    dbgprintf X ;
#endif

#define ipconfigHAS_PRINTF 0
#if ( ipconfigHAS_PRINTF == 1 )
#define FreeRTOS_printf( X )    dbgprintf X ;
#endif

#define ipconfigNETWORK_MTU 1514

#define ipconfigUSE_DHCP 0

#define ipconfigUSE_DNS_CACHE 0
#define ipconfigDNS_CACHE_NAME_LENGTH    254U

/* The QUICC doesnt have any special requirements for the number of buffers in
 * each ring, it will continually loop until the W flag is set. So both rings
 * may be essentially arbitrarily sized. A combined soft limit of 128 is
 * enforced, simply due to limitations of the amount of contiguous parameter
 * RAM available for storing the buffer descriptors while also leaving space
 * for other peripherals. */
#define ipconfigQUICC_RX_RING_SZ 8
#define ipconfigQUICC_TX_RING_SZ 4
#define ipconfigQUICC_BUFFERS_EXTRA 16

#define ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS (ipconfigQUICC_RX_RING_SZ + ipconfigQUICC_TX_RING_SZ + ipconfigQUICC_BUFFERS_EXTRA)
#define ipconfigEVENT_QUEUE_LENGTH (ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS + 16)

/* IP task properties */
#define ipconfigIP_TASK_PRIORITY (configMAX_PRIORITIES - 1)
#define ipconfigIP_TASK_STACK_SIZE_WORDS configMINIMAL_STACK_SIZE

/* M68k is big endian */
#define ipconfigBYTE_ORDER pdFREERTOS_BIG_ENDIAN

/* The QUICC takes care of checksumming the ethernet frame, but IP checksums
 * need to be calculated in software */
#define ipconfigDRIVER_INCLUDED_TX_IP_CHECKSUM 0
#define ipconfigDRIVER_INCLUDED_RX_IP_CHECKSUM 0

/* Use no-copy method of buffer management */
#define ipconfigZERO_COPY_TX_DRIVER 1
#define ipconfigZERO_COPY_RX_DRIVER 1

/* The QUICC requires its receive buffers to be aligned to 4 bytes (this doesnt
 * seem to apply to transmit buffers). The default amount of padding and filler
 * FreeRTOS+TCP adds to a packet buffer is 10 bytes, which means buffer
 * alignment is unlikely to meet the QUICCs requirements. Therefore, define this
 * value to be some multiple of 4 that is greater than 10. */
#define ipconfigBUFFER_PADDING 16

#endif	/* FREERTOS_IP_CONFIG_H */
