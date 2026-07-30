/* The counter of ../config/FreeRTOSConfig.h: every option a SolidSyslog adapter
 * gates on is off, so the gated-out path gets built. Compiled, never run. */
#ifndef SOLIDSYSLOG_CONSUMER_SMOKE_GATES_OFF_FREERTOS_CONFIG_H
#define SOLIDSYSLOG_CONSUMER_SMOKE_GATES_OFF_FREERTOS_CONFIG_H

#define configUSE_PREEMPTION 1
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configCPU_CLOCK_HZ ((unsigned long) 25000000)
#define configTICK_RATE_HZ ((TickType_t) 100)
#define configMAX_PRIORITIES 7
#define configMINIMAL_STACK_SIZE ((unsigned short) 128)
#define configTOTAL_HEAP_SIZE ((size_t) (16 * 1024))
#define configMAX_TASK_NAME_LEN 16
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1
#define configUSE_MUTEXES 0
#define configUSE_RECURSIVE_MUTEXES 0
#define configUSE_COUNTING_SEMAPHORES 1
#define configUSE_TIMERS 0
#define configCHECK_FOR_STACK_OVERFLOW 0
#define configUSE_MALLOC_FAILED_HOOK 0
#define configSUPPORT_STATIC_ALLOCATION 0
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configKERNEL_PROVIDED_STATIC_MEMORY 1
#define configUSE_CO_ROUTINES 0

#define INCLUDE_vTaskDelete 1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskGetSchedulerState 1

#define configPRIO_BITS 3
#define configKERNEL_INTERRUPT_PRIORITY (7 << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (5 << (8 - configPRIO_BITS))

#endif /* SOLIDSYSLOG_CONSUMER_SMOKE_GATES_OFF_FREERTOS_CONFIG_H */
