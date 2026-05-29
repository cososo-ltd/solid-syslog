#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* FreeRTOS kernel configuration for the Cortex-M3 QEMU mps2-an385 lwIP
 * link-probe (S28.07). Deliberately leaner than Bdd/Targets/FreeRtos/
 * FreeRTOSConfig.h: this image only has to link the kernel alongside lwIP
 * core + the Platform/LwipRaw adapters and start the scheduler with a single
 * probe task that _Create/_Destroys each adapter. No timers, no static
 * allocation, no IP-stack tasks, no hooks — so there is no hook boilerplate
 * to carry. S28.09 introduces the real netif + tcpip integration on top of
 * the fuller FreeRtos config. */

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
#define configUSE_COUNTING_SEMAPHORES 0
#define configUSE_TIMERS 0
#define configCHECK_FOR_STACK_OVERFLOW 0
#define configUSE_MALLOC_FAILED_HOOK 0
#define configSUPPORT_STATIC_ALLOCATION 0
#define configSUPPORT_DYNAMIC_ALLOCATION 1

#define configUSE_CO_ROUTINES 0
#define configMAX_CO_ROUTINE_PRIORITIES 1

#define INCLUDE_vTaskPrioritySet 0
#define INCLUDE_uxTaskPriorityGet 0
#define INCLUDE_vTaskDelete 0
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_vTaskDelayUntil 0
#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskGetSchedulerState 1

/* Cortex-M3 NVIC: top 3 priority bits implemented. */
#define configPRIO_BITS 3
#define configKERNEL_INTERRUPT_PRIORITY (7 << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (5 << (8 - configPRIO_BITS))

/* Alias FreeRTOS port handlers to the standard CMSIS names so the vector
 * table in Common/startup.c references SVC_Handler / PendSV_Handler /
 * SysTick_Handler (and the FreeRTOS port supplies the bodies). */
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
