#include "SolidSyslogPosixMutex.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPosixMutexErrors.h"
#include "SolidSyslogPosixMutexPrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogMutex;

static inline size_t PosixMutex_IndexFromHandle(const struct SolidSyslogMutex* base);
static inline void PosixMutex_CleanupAtIndex(size_t index, void* context);

static bool PosixMutex_InUse[SOLIDSYSLOG_MUTEX_POOL_SIZE];
static struct SolidSyslogPosixMutex PosixMutex_Pool[SOLIDSYSLOG_MUTEX_POOL_SIZE];
static struct SolidSyslogPoolAllocator PosixMutex_Allocator = {PosixMutex_InUse, SOLIDSYSLOG_MUTEX_POOL_SIZE};

struct SolidSyslogMutex* SolidSyslogPosixMutex_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&PosixMutex_Allocator);
    struct SolidSyslogMutex* handle = SolidSyslogNullMutex_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&PosixMutex_Allocator, index) == true)
    {
        PosixMutex_Initialise(&PosixMutex_Pool[index].Base);
        handle = &PosixMutex_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &PosixMutexErrorSource,
            (uint8_t) POSIXMUTEX_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogPosixMutex_Destroy(struct SolidSyslogMutex* base)
{
    size_t index = PosixMutex_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&PosixMutex_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(&PosixMutex_Allocator, index, PosixMutex_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &PosixMutexErrorSource,
            (uint8_t) POSIXMUTEX_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t PosixMutex_IndexFromHandle(const struct SolidSyslogMutex* base)
{
    size_t result = SOLIDSYSLOG_MUTEX_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_MUTEX_POOL_SIZE; poolIndex++)
    {
        if (base == &PosixMutex_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void PosixMutex_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    PosixMutex_Cleanup(&PosixMutex_Pool[index].Base);
}
