/* The FreeRTOS kernel's own callbacks, which no OS abstraction can cover: the
 * kernel calls these by name, and one of them takes a kernel type in its
 * signature. Every target that runs on this kernel ends them the same way, in
 * the board's fault line, so they are written once here. */

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
