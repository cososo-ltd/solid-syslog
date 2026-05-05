#include <FreeRTOS.h>
#include <task.h>

#include <stdio.h>

/* newlib rdimon stub — opens stdin/stdout/stderr via semihosting so that
 * printf is routed to QEMU's host stdout. */
extern void initialise_monitor_handles(void);

static void HelloTask(void* argument)
{
    (void) argument;
    printf("hello from FreeRTOS on QEMU mps2-an385\n");
    fflush(stdout);

    /* Block this task forever; the scheduler keeps running so QEMU stays
     * alive and is GDB-attachable. */
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    initialise_monitor_handles();

    if (xTaskCreate(HelloTask, "hello", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
    {
        /* Heap exhausted before the scheduler started — trap. */
        for (;;)
        {
        }
    }

    vTaskStartScheduler();

    /* Reached only if vTaskStartScheduler returned (idle / timer task could
     * not be allocated). */
    for (;;)
    {
    }
    return 0;
}

/* FreeRTOS hook: called from heap_4.c when pvPortMalloc fails. */
void vApplicationMallocFailedHook(void)
{
    for (;;)
    {
    }
}

/* FreeRTOS hook: called when a task overflows its stack
 * (configCHECK_FOR_STACK_OVERFLOW = 2). */
void vApplicationStackOverflowHook(TaskHandle_t task, char* taskName)
{
    (void) task;
    (void) taskName;
    for (;;)
    {
    }
}
