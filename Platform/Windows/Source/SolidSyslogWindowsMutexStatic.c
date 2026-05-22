#include "SolidSyslogWindowsMutex.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWindowsMutexPrivate.h"

struct SolidSyslogMutex;

static inline size_t WindowsMutex_IndexFromHandle(const struct SolidSyslogMutex* base);
static inline void WindowsMutex_CleanupAtIndex(size_t index, void* context);

static bool WindowsMutex_InUse[SOLIDSYSLOG_WINDOWS_MUTEX_POOL_SIZE];
static struct SolidSyslogWindowsMutex WindowsMutex_Pool[SOLIDSYSLOG_WINDOWS_MUTEX_POOL_SIZE];
static struct SolidSyslogPoolAllocator WindowsMutex_Allocator = {
    WindowsMutex_InUse,
    SOLIDSYSLOG_WINDOWS_MUTEX_POOL_SIZE
};

struct SolidSyslogMutex* SolidSyslogWindowsMutex_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&WindowsMutex_Allocator);
    struct SolidSyslogMutex* handle = SolidSyslogNullMutex_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&WindowsMutex_Allocator, index) == true)
    {
        WindowsMutex_Initialise(&WindowsMutex_Pool[index].Base);
        handle = &WindowsMutex_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_WINDOWSMUTEX_POOL_EXHAUSTED);
    }
    return handle;
}

void SolidSyslogWindowsMutex_Destroy(struct SolidSyslogMutex* base)
{
    size_t index = WindowsMutex_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&WindowsMutex_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&WindowsMutex_Allocator, index, WindowsMutex_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_WINDOWSMUTEX_UNKNOWN_DESTROY);
    }
}

static inline size_t WindowsMutex_IndexFromHandle(const struct SolidSyslogMutex* base)
{
    size_t result = SOLIDSYSLOG_WINDOWS_MUTEX_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_WINDOWS_MUTEX_POOL_SIZE; poolIndex++)
    {
        if (base == &WindowsMutex_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void WindowsMutex_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    WindowsMutex_Cleanup(&WindowsMutex_Pool[index].Base);
}
