#include "SolidSyslogLwipRawDnsResolver.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipRawDnsResolverErrors.h"
#include "SolidSyslogLwipRawDnsResolverPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogResolver;

static inline bool LwipRawDnsResolver_IsValidConfig(const struct SolidSyslogLwipRawDnsResolverConfig* config);
static inline size_t LwipRawDnsResolver_IndexFromHandle(const struct SolidSyslogResolver* base);
static inline void LwipRawDnsResolver_CleanupAtIndex(size_t index, void* context);

static bool LwipRawDnsResolver_InUse[SOLIDSYSLOG_RESOLVER_POOL_SIZE];
static struct SolidSyslogLwipRawDnsResolver LwipRawDnsResolver_Pool[SOLIDSYSLOG_RESOLVER_POOL_SIZE];
static struct SolidSyslogPoolAllocator LwipRawDnsResolver_Allocator = {
    LwipRawDnsResolver_InUse,
    SOLIDSYSLOG_RESOLVER_POOL_SIZE
};

struct SolidSyslogResolver* SolidSyslogLwipRawDnsResolver_Create(
    const struct SolidSyslogLwipRawDnsResolverConfig* config
)
{
    struct SolidSyslogResolver* handle = SolidSyslogNullResolver_Get();
    if (LwipRawDnsResolver_IsValidConfig(config))
    {
        size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&LwipRawDnsResolver_Allocator);
        if (SolidSyslogPoolAllocator_IndexIsValid(&LwipRawDnsResolver_Allocator, index) == true)
        {
            LwipRawDnsResolver_Initialise(&LwipRawDnsResolver_Pool[index].Base, config);
            handle = &LwipRawDnsResolver_Pool[index].Base;
        }
        else
        {
            SolidSyslog_Error(
                SOLIDSYSLOG_SEVERITY_ERROR,
                &LwipRawDnsResolverErrorSource,
                (uint8_t) LWIPRAWDNSRESOLVER_ERROR_POOL_EXHAUSTED
            );
        }
    }
    return handle;
}

void SolidSyslogLwipRawDnsResolver_Destroy(struct SolidSyslogResolver* base)
{
    size_t index = LwipRawDnsResolver_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&LwipRawDnsResolver_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &LwipRawDnsResolver_Allocator,
                        index,
                        LwipRawDnsResolver_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &LwipRawDnsResolverErrorSource,
            (uint8_t) LWIPRAWDNSRESOLVER_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline bool LwipRawDnsResolver_IsValidConfig(const struct SolidSyslogLwipRawDnsResolverConfig* config)
{
    return (config != NULL) && (config->Sleep != NULL);
}

static inline size_t LwipRawDnsResolver_IndexFromHandle(const struct SolidSyslogResolver* base)
{
    size_t result = SOLIDSYSLOG_RESOLVER_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_RESOLVER_POOL_SIZE; poolIndex++)
    {
        if (base == &LwipRawDnsResolver_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void LwipRawDnsResolver_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    LwipRawDnsResolver_Cleanup(&LwipRawDnsResolver_Pool[index].Base);
}
