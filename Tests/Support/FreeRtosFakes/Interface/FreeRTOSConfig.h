#ifndef SOLIDSYSLOG_TESTS_FREERTOSFAKES_FREERTOSCONFIG_H
#define SOLIDSYSLOG_TESTS_FREERTOSFAKES_FREERTOSCONFIG_H

/* FreeRTOS-Plus-TCP's IPConfig defaults header checks this guard to confirm
 * FreeRTOSConfig.h was included before any IP header. */
#define FREERTOS_CONFIG_H

/* Host-suitable FreeRTOSConfig.h for compiling Platform/FreeRtos adapters
 * against fakes. The values here are chosen so the real FreeRTOS-Kernel
 * headers parse cleanly on the host compiler — actual scheduler behaviour
 * is provided by FreeRtosFakes/Source/, which substitutes the API at link
 * time.
 *
 * No test source compiles against this at S08.01. First content lands with
 * S08.04 (semaphore fake for SolidSyslogFreeRtosMutex).
 */

#define configUSE_PREEMPTION 1
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configCPU_CLOCK_HZ ((unsigned long) 25000000UL)
#define configTICK_RATE_HZ ((TickType_t) 100)
#define configMAX_PRIORITIES 5
#define configMINIMAL_STACK_SIZE ((unsigned short) 128)
#define configTOTAL_HEAP_SIZE ((size_t) (16 * 1024))
#define configMAX_TASK_NAME_LEN 16
#define configUSE_TRACE_FACILITY 0
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1
#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 0
#define configUSE_COUNTING_SEMAPHORES 1
#define configUSE_TIMERS 0
#define configCHECK_FOR_STACK_OVERFLOW 0
#define configUSE_MALLOC_FAILED_HOOK 0

/* Static allocation is required for SolidSyslogFreeRtosMutex
 * (xSemaphoreCreateMutexStatic) — caller injects the StaticSemaphore_t-sized
 * storage rather than the kernel mallocing it. Dynamic stays on so the
 * test config matches the example, which also uses dynamic allocation for
 * task / FreeRTOS-Plus-TCP buffers. */
#define configSUPPORT_STATIC_ALLOCATION 1
#define configSUPPORT_DYNAMIC_ALLOCATION 1

/* Co-routines and software timers are off in tests; saves headers from
 * pulling in extra symbols that fakes would have to satisfy. */
#define configUSE_CO_ROUTINES 0
#define configMAX_CO_ROUTINE_PRIORITIES 1

/* Optional API inclusions. */
#define INCLUDE_vTaskPrioritySet 0
#define INCLUDE_uxTaskPriorityGet 0
#define INCLUDE_vTaskDelete 0
#define INCLUDE_vTaskCleanUpResources 0
#define INCLUDE_vTaskSuspend 0
#define INCLUDE_vTaskDelayUntil 0
#define INCLUDE_vTaskDelay 1

/* Assert: route to a host-friendly trap that the test can detect. */
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

#endif /* SOLIDSYSLOG_TESTS_FREERTOSFAKES_FREERTOSCONFIG_H */
