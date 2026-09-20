/* The FreeRTOS implementation of the BDD-target OS seam - see
 * BddTargetOsPrimitives.h. Selected by the two targets that speak the kernel
 * API directly; the CMSIS-RTOS2 sibling is selected by the target that speaks
 * the wrapper instead. */

#include "BddTargetOsPrimitives.h"

#include "SolidSyslogFreeRtosMutex.h"
#include "SolidSyslogFreeRtosSysUpTime.h"

#include <FreeRTOS.h>
#include <task.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* One level above idle - see the priority note in BddTargetOsPrimitives.h. */
#define BDD_TARGET_THREAD_PRIORITY (tskIDLE_PRIORITY + 1U)

struct SolidSyslogMutex* BddTargetOsPrimitives_CreateMutex(unsigned slot)
{
    /* FreeRTOS places the control block inside the adapter's own pool slot, so
     * the caller supplies no storage and the slots are interchangeable. */
    (void) slot;
    return SolidSyslogFreeRtosMutex_Create();
}

void BddTargetOsPrimitives_DestroyMutex(struct SolidSyslogMutex* mutex)
{
    SolidSyslogFreeRtosMutex_Destroy(mutex);
}

uint32_t BddTargetOsPrimitives_GetSysUpTime(void)
{
    return SolidSyslogFreeRtos_GetSysUpTime();
}

void BddTargetOsPrimitives_InitialiseOs(void)
{
    /* The kernel needs no call before xTaskCreate; vTaskStartScheduler does
     * the initialising. */
}

bool BddTargetOsPrimitives_Spawn(void (*entry)(void* argument), const char* name, uint32_t stackBytes)
{
    configSTACK_DEPTH_TYPE depth = (configSTACK_DEPTH_TYPE) (stackBytes / sizeof(StackType_t));
    return xTaskCreate(entry, name, depth, NULL, BDD_TARGET_THREAD_PRIORITY, NULL) == pdPASS;
}

void BddTargetOsPrimitives_StartScheduler(void)
{
    vTaskStartScheduler();
}

BddTargetThread BddTargetOsPrimitives_CurrentThread(void)
{
    return (BddTargetThread) xTaskGetCurrentTaskHandle();
}

void BddTargetOsPrimitives_ExitThread(void)
{
    vTaskDelete(NULL);
}

uint32_t BddTargetOsPrimitives_StackHeadroomBytes(BddTargetThread thread)
{
    uint32_t headroom = 0U;
    if (thread != NULL)
    {
        /* uxTaskGetStackHighWaterMark counts StackType_t words. */
        headroom = (uint32_t) uxTaskGetStackHighWaterMark((TaskHandle_t) thread) * (uint32_t) sizeof(StackType_t);
    }
    return headroom;
}

void BddTargetOsPrimitives_Notify(BddTargetThread thread)
{
    if (thread != NULL)
    {
        (void) xTaskNotifyGive((TaskHandle_t) thread);
    }
}

bool BddTargetOsPrimitives_WaitForNotify(uint32_t timeoutMilliseconds)
{
    return ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeoutMilliseconds)) != 0U;
}

void BddTargetOsPrimitives_Sleep(int milliseconds)
{
    /* Round a non-zero sub-tick request up to one tick, or vTaskDelay(0) would
     * only yield - the CmsdkUart yield against a 100 Hz tick relies on this. */
    TickType_t ticks = pdMS_TO_TICKS((TickType_t) milliseconds);
    if ((milliseconds > 0) && (ticks == 0U))
    {
        ticks = 1U;
    }
    vTaskDelay(ticks);
}

uint64_t BddTargetOsPrimitives_UptimeMilliseconds(void)
{
    return (uint64_t) xTaskGetTickCount() * (uint64_t) portTICK_PERIOD_MS;
}
