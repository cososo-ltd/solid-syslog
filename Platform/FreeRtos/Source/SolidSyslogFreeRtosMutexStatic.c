#include "SolidSyslogFreeRtosMutex.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogFreeRtosMutexPrivate.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogMutex;

static inline size_t FreeRtosMutex_IndexFromHandle(const struct SolidSyslogMutex* base);
static inline void FreeRtosMutex_CleanupAtIndex(size_t index, void* context);

static bool FreeRtosMutex_InUse[SOLIDSYSLOG_FREE_RTOS_MUTEX_POOL_SIZE];
static struct SolidSyslogFreeRtosMutex FreeRtosMutex_Pool[SOLIDSYSLOG_FREE_RTOS_MUTEX_POOL_SIZE];
static struct SolidSyslogPoolAllocator FreeRtosMutex_Allocator = {
    FreeRtosMutex_InUse,
    SOLIDSYSLOG_FREE_RTOS_MUTEX_POOL_SIZE
};

struct SolidSyslogMutex* SolidSyslogFreeRtosMutex_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&FreeRtosMutex_Allocator);
    struct SolidSyslogMutex* handle = SolidSyslogNullMutex_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&FreeRtosMutex_Allocator, index))
    {
        FreeRtosMutex_Initialise(&FreeRtosMutex_Pool[index].Base);
        handle = &FreeRtosMutex_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_FREERTOSMUTEX_POOL_EXHAUSTED);
    }
    return handle;
}

void SolidSyslogFreeRtosMutex_Destroy(struct SolidSyslogMutex* base)
{
    size_t index = FreeRtosMutex_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&FreeRtosMutex_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&FreeRtosMutex_Allocator, index, FreeRtosMutex_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_FREERTOSMUTEX_UNKNOWN_DESTROY);
    }
}

static inline size_t FreeRtosMutex_IndexFromHandle(const struct SolidSyslogMutex* base)
{
    size_t result = SOLIDSYSLOG_FREE_RTOS_MUTEX_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_FREE_RTOS_MUTEX_POOL_SIZE; poolIndex++)
    {
        if (base == &FreeRtosMutex_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void FreeRtosMutex_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    FreeRtosMutex_Cleanup(&FreeRtosMutex_Pool[index].Base);
}
