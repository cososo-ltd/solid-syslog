#include "SolidSyslogLwipRawAddress.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogLwipRawAddressErrors.h"
#include "SolidSyslogLwipRawAddressPrivate.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogAddress;

static inline struct SolidSyslogAddress* LwipRawAddress_HandleFromIndex(size_t index);
static inline size_t LwipRawAddress_IndexFromHandle(const struct SolidSyslogAddress* base);
static inline void LwipRawAddress_CleanupAtIndex(size_t index, void* context);

static bool LwipRawAddress_InUse[SOLIDSYSLOG_ADDRESS_POOL_SIZE];
static struct SolidSyslogPoolAllocator LwipRawAddress_Allocator = {LwipRawAddress_InUse, SOLIDSYSLOG_ADDRESS_POOL_SIZE};

struct SolidSyslogAddress* SolidSyslogLwipRawAddress_Create(void)
{
    /* TU-private fallback returned when the pool is exhausted. Sized as
     * a real SolidSyslogLwipRawAddress so subsequent writes (e.g. a Resolver
     * overwrite at the exhausted-fallback call site) are bounded — same
     * ip_addr_t + u16_t storage as any pooled slot. Not a per-Sender slot:
     * multi-overflow integrators share this storage and race on it. Bumping
     * SOLIDSYSLOG_ADDRESS_POOL_SIZE removes the race. */
    static struct SolidSyslogLwipRawAddress fallback;

    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&LwipRawAddress_Allocator);
    struct SolidSyslogAddress* handle = (struct SolidSyslogAddress*) &fallback;
    if (SolidSyslogPoolAllocator_IndexIsValid(&LwipRawAddress_Allocator, index) == true)
    {
        handle = LwipRawAddress_HandleFromIndex(index);
        LwipRawAddress_Initialise(handle);
    }
    else
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &LwipRawAddressErrorSource,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            (int32_t) LWIPRAWADDRESS_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

static inline struct SolidSyslogAddress* LwipRawAddress_HandleFromIndex(size_t index)
{
    static struct SolidSyslogLwipRawAddress pool[SOLIDSYSLOG_ADDRESS_POOL_SIZE];
    return (struct SolidSyslogAddress*) &pool[index];
}

void SolidSyslogLwipRawAddress_Destroy(struct SolidSyslogAddress* base)
{
    size_t index = LwipRawAddress_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&LwipRawAddress_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&LwipRawAddress_Allocator, index, LwipRawAddress_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &LwipRawAddressErrorSource,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            (int32_t) LWIPRAWADDRESS_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t LwipRawAddress_IndexFromHandle(const struct SolidSyslogAddress* base)
{
    size_t result = SOLIDSYSLOG_ADDRESS_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_ADDRESS_POOL_SIZE; poolIndex++)
    {
        if (base == LwipRawAddress_HandleFromIndex(poolIndex))
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void LwipRawAddress_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    LwipRawAddress_Cleanup(LwipRawAddress_HandleFromIndex(index));
}
