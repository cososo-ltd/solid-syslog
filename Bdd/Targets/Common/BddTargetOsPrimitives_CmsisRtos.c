/* The CMSIS-RTOS2 implementation of the BDD-target OS seam - see
 * BddTargetOsPrimitives.h. Selected by the target that speaks the wrapper
 * rather than the kernel beneath it, and the reason nothing in the shared
 * pipeline names a kernel. */

#include "BddTargetOsPrimitives.h"

#include "SolidSyslogCmsisRtosMutex.h"
#include "SolidSyslogCmsisRtosSysUpTime.h"

#include "cmsis_os2.h"

#include <FreeRTOS.h>
#include <semphr.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* One flag is enough: only the Service thread signals, and only the thread
 * tearing down waits. */
#define BDD_TARGET_STOP_FLAG 0x00000001U

/* CMSIS-RTOS2 does not standardise the size of a mutex control block, so only
 * the integrator can supply one - see SolidSyslogCmsisRtosMutex.h. On this
 * kernel osMutexNew routes the storage to xSemaphoreCreateMutexStatic, which
 * needs a StaticSemaphore_t. One per slot, so no running total decides which
 * block a given mutex got. */
static StaticSemaphore_t controlBlocks[BDD_TARGET_MUTEX_COUNT];

struct SolidSyslogMutex* BddTargetOsPrimitives_CreateMutex(unsigned slot)
{
    return SolidSyslogCmsisRtosMutex_Create(&controlBlocks[slot], (uint32_t) sizeof(controlBlocks[slot]));
}

void BddTargetOsPrimitives_DestroyMutex(struct SolidSyslogMutex* mutex)
{
    SolidSyslogCmsisRtosMutex_Destroy(mutex);
}

uint32_t BddTargetOsPrimitives_GetSysUpTime(void)
{
    return SolidSyslogCmsisRtos_GetSysUpTime();
}

void BddTargetOsPrimitives_InitialiseOs(void)
{
    (void) osKernelInitialize();
}

bool BddTargetOsPrimitives_Spawn(void (*entry)(void* argument), const char* name, uint32_t stackBytes)
{
    /* stack_size is in bytes here, which is what this seam already speaks -
     * osThreadNew divides it down to the kernel's own stack units itself.
     * Normal priority puts these below the network threads and above idle,
     * which is the ordering the target has always run. */
    osThreadAttr_t attributes = {0};
    attributes.name = name;
    attributes.stack_size = stackBytes;
    attributes.priority = osPriorityNormal;
    return osThreadNew(entry, NULL, &attributes) != NULL;
}

void BddTargetOsPrimitives_StartScheduler(void)
{
    (void) osKernelStart();
}

BddTargetThread BddTargetOsPrimitives_CurrentThread(void)
{
    return (BddTargetThread) osThreadGetId();
}

void BddTargetOsPrimitives_ExitThread(void)
{
    osThreadExit();
}

uint32_t BddTargetOsPrimitives_StackHeadroomBytes(BddTargetThread thread)
{
    uint32_t headroom = 0U;
    if (thread != NULL)
    {
        /* osThreadGetStackSpace already reports bytes. */
        headroom = osThreadGetStackSpace((osThreadId_t) thread);
    }
    return headroom;
}

void BddTargetOsPrimitives_Notify(BddTargetThread thread)
{
    if (thread != NULL)
    {
        (void) osThreadFlagsSet((osThreadId_t) thread, BDD_TARGET_STOP_FLAG);
    }
}

bool BddTargetOsPrimitives_WaitForNotify(uint32_t timeoutMilliseconds)
{
    /* A timeout, and every error, come back with the high bit set; a success
     * returns the flags that were signalled. */
    uint32_t result = osThreadFlagsWait(BDD_TARGET_STOP_FLAG, osFlagsWaitAny, timeoutMilliseconds);
    return (result & osFlagsError) == 0U;
}

void BddTargetOsPrimitives_Sleep(int milliseconds)
{
    /* Round a non-zero sub-tick request up to one tick, or osDelay(0) would
     * only yield - the CmsdkUart yield against a 100 Hz tick relies on this. */
    uint32_t ticks = ((uint32_t) milliseconds * osKernelGetTickFreq()) / 1000U;
    if ((milliseconds > 0) && (ticks == 0U))
    {
        ticks = 1U;
    }
    (void) osDelay(ticks);
}

uint64_t BddTargetOsPrimitives_UptimeMilliseconds(void)
{
    return ((uint64_t) osKernelGetTickCount() * 1000U) / (uint64_t) osKernelGetTickFreq();
}
