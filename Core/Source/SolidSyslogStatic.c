#include "SolidSyslogConfig.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogNullSender.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrivate.h"
#include "SolidSyslogTunables.h"

static inline size_t SolidSyslog_IndexFromHandle(const struct SolidSyslog* handle);
static inline void SolidSyslog_CleanupAtIndex(size_t index, void* context);
static void SolidSyslog_EnsureNullInstancePopulated(void);

static bool SolidSyslog_InUse[SOLIDSYSLOG_POOL_SIZE];
static struct SolidSyslog SolidSyslog_Pool[SOLIDSYSLOG_POOL_SIZE];
static struct SolidSyslogPoolAllocator SolidSyslog_Allocator = {SolidSyslog_InUse, SOLIDSYSLOG_POOL_SIZE};

/* Exhaustion-fallback handle. Populated lazily on first reach because
 * SolidSyslogNull*_Get() returns runtime addresses (no file-scope designated
 * initialiser will accept them). Sits outside SolidSyslog_Pool[] so
 * IndexFromHandle naturally returns invalid for it — _Destroy(&NullInstance)
 * fires WARNING + ignore, while _Log/_Service against it route through the
 * public Null* siblings and silently drop. */
static struct SolidSyslog SolidSyslog_NullInstance;

struct SolidSyslog* SolidSyslog_Create(const struct SolidSyslogConfig* config)
{
    SolidSyslog_EnsureNullInstancePopulated();
    struct SolidSyslog* result = &SolidSyslog_NullInstance;
    if (config == NULL)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &SolidSyslogErrorSource,
            (uint8_t) SOLIDSYSLOG_ERROR_CREATE_NULL_CONFIG
        );
    }
    else
    {
        size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&SolidSyslog_Allocator);
        if (SolidSyslogPoolAllocator_IndexIsValid(&SolidSyslog_Allocator, index))
        {
            SolidSyslog_Initialise(&SolidSyslog_Pool[index], config);
            result = &SolidSyslog_Pool[index];
        }
        else
        {
            SolidSyslog_Error(
                SOLIDSYSLOG_SEVERITY_ERROR,
                &SolidSyslogErrorSource,
                (uint8_t) SOLIDSYSLOG_ERROR_POOL_EXHAUSTED
            );
        }
    }
    return result;
}

static void SolidSyslog_EnsureNullInstancePopulated(void)
{
    static bool populated = false;
    if (!populated)
    {
        SolidSyslog_NullInstance.Buffer = SolidSyslogNullBuffer_Get();
        SolidSyslog_NullInstance.Sender = SolidSyslogNullSender_Get();
        SolidSyslog_NullInstance.Store = SolidSyslogNullStore_Get();
        SolidSyslog_NullInstance.Clock = SolidSyslog_NullClock;
        SolidSyslog_NullInstance.GetHostname = SolidSyslog_NullStringFunction;
        SolidSyslog_NullInstance.GetAppName = SolidSyslog_NullStringFunction;
        SolidSyslog_NullInstance.GetProcessId = SolidSyslog_NullStringFunction;
        SolidSyslog_NullInstance.Sd = NULL;
        SolidSyslog_NullInstance.SdCount = 0;
        populated = true;
    }
}

void SolidSyslog_Destroy(struct SolidSyslog* handle)
{
    size_t index = SolidSyslog_IndexFromHandle(handle);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&SolidSyslog_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&SolidSyslog_Allocator, index, SolidSyslog_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &SolidSyslogErrorSource,
            (uint8_t) SOLIDSYSLOG_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t SolidSyslog_IndexFromHandle(const struct SolidSyslog* handle)
{
    size_t result = SOLIDSYSLOG_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_POOL_SIZE; poolIndex++)
    {
        if (handle == &SolidSyslog_Pool[poolIndex])
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void SolidSyslog_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslog_Cleanup(&SolidSyslog_Pool[index]);
}
