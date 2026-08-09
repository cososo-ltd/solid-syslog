#include "SolidSyslogStdAtomicCounter.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullAtomicCounter.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStdAtomicCounterErrors.h"
#include "SolidSyslogStdAtomicCounterPrivate.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogAtomicCounter;

static inline size_t StdAtomicCounter_IndexFromHandle(const struct SolidSyslogAtomicCounter* base);
static inline void StdAtomicCounter_CleanupAtIndex(size_t index, void* context);

static bool StdAtomicCounter_InUse[SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE];
static struct SolidSyslogStdAtomicCounter StdAtomicCounter_Pool[SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE];
static struct SolidSyslogPoolAllocator StdAtomicCounter_Allocator = {
    StdAtomicCounter_InUse,
    SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE
};

struct SolidSyslogAtomicCounter* SolidSyslogStdAtomicCounter_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&StdAtomicCounter_Allocator);
    struct SolidSyslogAtomicCounter* handle = SolidSyslogNullAtomicCounter_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&StdAtomicCounter_Allocator, index) == true)
    {
        StdAtomicCounter_Initialise(&StdAtomicCounter_Pool[index].Base);
        handle = &StdAtomicCounter_Pool[index].Base;
    }
    else
    {
        StdAtomicCounter_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            STDATOMICCOUNTER_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogStdAtomicCounter_Destroy(struct SolidSyslogAtomicCounter* base)
{
    size_t index = StdAtomicCounter_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&StdAtomicCounter_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&StdAtomicCounter_Allocator, index, StdAtomicCounter_CleanupAtIndex, NULL);
    if (!released)
    {
        StdAtomicCounter_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            STDATOMICCOUNTER_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t StdAtomicCounter_IndexFromHandle(const struct SolidSyslogAtomicCounter* base)
{
    size_t result = SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE; poolIndex++)
    {
        if (base == &StdAtomicCounter_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void StdAtomicCounter_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    StdAtomicCounter_Cleanup(&StdAtomicCounter_Pool[index].Base);
}
