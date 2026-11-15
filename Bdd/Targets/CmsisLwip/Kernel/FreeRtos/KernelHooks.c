/* The kernel's own callbacks, which no OS abstraction can cover: FreeRTOS
 * calls these by name, and one of them takes a FreeRTOS type in its signature.
 * They live beside the kernel config rather than in main.c so that everything
 * tied to this particular kernel sits in one directory - see Kernel.cmake. */

#include "BddTargetBoard.h"

#include <FreeRTOS.h>
#include <task.h>

void vApplicationMallocFailedHook(void)
{
    BddTargetBoard_Fault("kernel heap", "exhausted");
}

void vApplicationStackOverflowHook(TaskHandle_t task, char* taskName)
{
    (void) task;
    BddTargetBoard_Fault("stack overflow in task", taskName);
}
