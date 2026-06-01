#include "SolidSyslogWindowsAtomicCounter.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogNullAtomicCounter.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWindowsAtomicCounterErrors.h"
#include "SolidSyslogWindowsAtomicCounterPrivate.h"

struct SolidSyslogAtomicCounter;

static inline size_t WindowsAtomicCounter_IndexFromHandle(const struct SolidSyslogAtomicCounter* base);
static inline void WindowsAtomicCounter_CleanupAtIndex(size_t index, void* context);

static bool WindowsAtomicCounter_InUse[SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE];
static struct SolidSyslogWindowsAtomicCounter WindowsAtomicCounter_Pool[SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE];
static struct SolidSyslogPoolAllocator WindowsAtomicCounter_Allocator = {
    WindowsAtomicCounter_InUse,
    SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE
};

struct SolidSyslogAtomicCounter* SolidSyslogWindowsAtomicCounter_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&WindowsAtomicCounter_Allocator);
    struct SolidSyslogAtomicCounter* handle = SolidSyslogNullAtomicCounter_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&WindowsAtomicCounter_Allocator, index) == true)
    {
        WindowsAtomicCounter_Initialise(&WindowsAtomicCounter_Pool[index].Base);
        handle = &WindowsAtomicCounter_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &WindowsAtomicCounterErrorSource,
            (uint8_t) WINDOWSATOMICCOUNTER_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogWindowsAtomicCounter_Destroy(struct SolidSyslogAtomicCounter* base)
{
    size_t index = WindowsAtomicCounter_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&WindowsAtomicCounter_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &WindowsAtomicCounter_Allocator,
                        index,
                        WindowsAtomicCounter_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &WindowsAtomicCounterErrorSource,
            (uint8_t) WINDOWSATOMICCOUNTER_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t WindowsAtomicCounter_IndexFromHandle(const struct SolidSyslogAtomicCounter* base)
{
    size_t result = SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE; poolIndex++)
    {
        if (base == &WindowsAtomicCounter_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void WindowsAtomicCounter_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    WindowsAtomicCounter_Cleanup(&WindowsAtomicCounter_Pool[index].Base);
}
