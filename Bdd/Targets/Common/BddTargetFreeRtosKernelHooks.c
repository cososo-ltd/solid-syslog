/* The FreeRTOS kernel calls these by name, and one takes a kernel type, so no
 * OS abstraction can cover them. Every target on this kernel reports them
 * through BddTargetBoard_Fault, so they are defined once here. */

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
