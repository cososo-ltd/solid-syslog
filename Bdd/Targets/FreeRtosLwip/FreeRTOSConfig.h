#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* FreeRTOS kernel configuration for Cortex-M3 on QEMU mps2-an385 with lwIP
 * (Raw API) under NO_SYS=0. lwIP runs its own "tcpip" thread (priority and
 * stack sized in lwipopts.h via the contrib FreeRTOS sys_arch), the LAN9118
 * netif RX task at configMAX_PRIORITIES - 2, and the SolidSyslog interactive /
 * service tasks at tskIDLE_PRIORITY + 1.
 *
 * S28.09 grew this from the S28.07 link-probe config: the lwIP FreeRTOS
 * sys_arch needs recursive mutexes (the LWIP_TCPIP_CORE_LOCKING mutex) and
 * counting semaphores, and SolidSyslogFreeRtosMutex needs static allocation -
 * so this now mirrors Bdd/Targets/FreeRtos/FreeRTOSConfig.h (the proven
 * networking config for this machine) rather than the lean probe variant. The
 * FreeRTOS software-timer service is left ON for parity even though lwIP runs
 * its timeouts on the tcpip thread, not FreeRTOS timers. */

#define configUSE_PREEMPTION 1
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configCPU_CLOCK_HZ ((unsigned long) 25000000)
#define configTICK_RATE_HZ ((TickType_t) 100)
#define configMAX_PRIORITIES 7
#define configMINIMAL_STACK_SIZE ((unsigned short) 128)
#define configTOTAL_HEAP_SIZE ((size_t) (96 * 1024))
#define configMAX_TASK_NAME_LEN 16
#define configUSE_TRACE_FACILITY 0
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1
#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 1
#define configUSE_COUNTING_SEMAPHORES 1
#define configUSE_TIMERS 1
#define configTIMER_TASK_PRIORITY (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH 8
#define configTIMER_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE * 2)
#define configCHECK_FOR_STACK_OVERFLOW 2
#define configUSE_MALLOC_FAILED_HOOK 1
/* Static allocation is required by SolidSyslogFreeRtosMutex
 * (xSemaphoreCreateMutexStatic places the StaticSemaphore_t inside the
 * caller-supplied storage). The idle / timer task static-memory hooks it
 * pulls in are satisfied by configKERNEL_PROVIDED_STATIC_MEMORY = 1 - no
 * boilerplate in main.c. Dynamic allocation stays on for the lwIP tcpip /
 * RX tasks and the interactive / service tasks created via xTaskCreate. */
#define configSUPPORT_STATIC_ALLOCATION 1
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configKERNEL_PROVIDED_STATIC_MEMORY 1

#define configUSE_CO_ROUTINES 0
#define configMAX_CO_ROUTINE_PRIORITIES 1

#define INCLUDE_vTaskPrioritySet 1
#define INCLUDE_uxTaskPriorityGet 1
#define INCLUDE_vTaskDelete 1
#define INCLUDE_vTaskCleanUpResources 0
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_xTimerPendFunctionCall 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1

/* Cortex-M3 NVIC: top 3 priority bits implemented. */
#define configPRIO_BITS 3
#define configKERNEL_INTERRUPT_PRIORITY (7 << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (5 << (8 - configPRIO_BITS))
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 7
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

/* Alias FreeRTOS port handlers to the standard CMSIS names so the vector
 * table in Startup.c references SVC_Handler / PendSV_Handler / SysTick_Handler
 * (and the FreeRTOS port supplies the bodies). */
#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#define configASSERT(x) \
    do                  \
    {                   \
        if (!(x))       \
        {               \
            for (;;)    \
            {           \
            }           \
        }               \
    } while (0)

#endif /* FREERTOS_CONFIG_H */
