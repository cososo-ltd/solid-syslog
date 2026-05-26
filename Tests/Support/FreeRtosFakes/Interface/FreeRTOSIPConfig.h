#ifndef FREERTOS_IP_CONFIG_H
#define FREERTOS_IP_CONFIG_H

#include "FreeRTOS.h"

/* Host-suitable FreeRTOS-Plus-TCP config for unit-test fakes. The IP stack
 * itself is never run on the host — these values are only here so the
 * upstream Plus-TCP headers parse cleanly. Real behaviour comes from
 * FreeRtosFakes/Source/, which substitutes the API at link time.
 */

#define ipconfigUSE_TCP 1
#define ipconfigUSE_DHCP 0
#define ipconfigUSE_DNS 1
#define ipconfigEVENT_QUEUE_LENGTH 70
#define ipconfigNETWORK_MTU 1500
#define ipconfigUSE_NETWORK_EVENT_HOOK 0
#define ipconfigIP_TASK_PRIORITY (configMAX_PRIORITIES - 2)
#define ipconfigIP_TASK_STACK_SIZE_WORDS (configMINIMAL_STACK_SIZE * 5)
#define ipconfigBYTE_ORDER pdFREERTOS_LITTLE_ENDIAN
#define ipconfigZERO_COPY_RX_DRIVER 0
#define ipconfigZERO_COPY_TX_DRIVER 0

#endif /* FREERTOS_IP_CONFIG_H */
