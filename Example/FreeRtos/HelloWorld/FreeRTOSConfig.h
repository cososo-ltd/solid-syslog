#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* FreeRTOS kernel configuration for Cortex-M3 on QEMU mps2-an385. Tuned for
 * a hello-world smoke test — heap 32 KiB, 5 priorities, 100 Hz tick. */

#define configUSE_PREEMPTION 1
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configCPU_CLOCK_HZ ((unsigned long) 25000000)
#define configTICK_RATE_HZ ((TickType_t) 100)
#define configMAX_PRIORITIES 5
#define configMINIMAL_STACK_SIZE ((unsigned short) 128)
#define configTOTAL_HEAP_SIZE ((size_t) (32 * 1024))
#define configMAX_TASK_NAME_LEN 16
#define configUSE_TRACE_FACILITY 0
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1
#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 0
#define configUSE_COUNTING_SEMAPHORES 1
#define configUSE_TIMERS 0
#define configCHECK_FOR_STACK_OVERFLOW 2
#define configUSE_MALLOC_FAILED_HOOK 1
#define configSUPPORT_STATIC_ALLOCATION 0
#define configSUPPORT_DYNAMIC_ALLOCATION 1

#define configUSE_CO_ROUTINES 0
#define configMAX_CO_ROUTINE_PRIORITIES 1

#define INCLUDE_vTaskPrioritySet 0
#define INCLUDE_uxTaskPriorityGet 0
#define INCLUDE_vTaskDelete 0
#define INCLUDE_vTaskCleanUpResources 0
#define INCLUDE_vTaskSuspend 0
#define INCLUDE_vTaskDelayUntil 0
#define INCLUDE_vTaskDelay 1

/* Cortex-M3 NVIC: top 3 priority bits implemented. */
#define configPRIO_BITS 3
#define configKERNEL_INTERRUPT_PRIORITY (7 << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (5 << (8 - configPRIO_BITS))
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 7
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

/* Alias FreeRTOS port handlers to the standard CMSIS names so the vector
 * table in startup.c references SVC_Handler / PendSV_Handler / SysTick_Handler
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
