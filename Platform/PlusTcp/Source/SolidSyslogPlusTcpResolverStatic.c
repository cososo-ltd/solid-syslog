#include "SolidSyslogPlusTcpResolver.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPlusTcpResolverErrors.h"
#include "SolidSyslogPlusTcpResolverPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogResolver;

static inline size_t PlusTcpResolver_IndexFromHandle(const struct SolidSyslogResolver* base);
static inline void PlusTcpResolver_CleanupAtIndex(size_t index, void* context);

static bool PlusTcpResolver_InUse[SOLIDSYSLOG_RESOLVER_POOL_SIZE];
static struct SolidSyslogPlusTcpResolver PlusTcpResolver_Pool[SOLIDSYSLOG_RESOLVER_POOL_SIZE];
static struct SolidSyslogPoolAllocator PlusTcpResolver_Allocator = {
    PlusTcpResolver_InUse,
    SOLIDSYSLOG_RESOLVER_POOL_SIZE
};

struct SolidSyslogResolver* SolidSyslogPlusTcpResolver_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&PlusTcpResolver_Allocator);
    struct SolidSyslogResolver* handle = SolidSyslogNullResolver_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&PlusTcpResolver_Allocator, index) == true)
    {
        PlusTcpResolver_Initialise(&PlusTcpResolver_Pool[index].Base);
        handle = &PlusTcpResolver_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &PlusTcpResolverErrorSource,
            (uint8_t) PLUSTCPRESOLVER_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogPlusTcpResolver_Destroy(struct SolidSyslogResolver* base)
{
    size_t index = PlusTcpResolver_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&PlusTcpResolver_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&PlusTcpResolver_Allocator, index, PlusTcpResolver_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &PlusTcpResolverErrorSource,
            (uint8_t) PLUSTCPRESOLVER_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t PlusTcpResolver_IndexFromHandle(const struct SolidSyslogResolver* base)
{
    size_t result = SOLIDSYSLOG_RESOLVER_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_RESOLVER_POOL_SIZE; poolIndex++)
    {
        if (base == &PlusTcpResolver_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void PlusTcpResolver_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    PlusTcpResolver_Cleanup(&PlusTcpResolver_Pool[index].Base);
}
