#include "SolidSyslogPlusTcpAddress.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPlusTcpAddressErrors.h"
#include "SolidSyslogPlusTcpAddressPrivate.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogAddress;

static inline struct SolidSyslogAddress* PlusTcpAddress_HandleFromIndex(size_t index);
static inline size_t PlusTcpAddress_IndexFromHandle(const struct SolidSyslogAddress* base);
static inline void PlusTcpAddress_CleanupAtIndex(size_t index, void* context);

static bool PlusTcpAddress_InUse[SOLIDSYSLOG_ADDRESS_POOL_SIZE];
static struct SolidSyslogPoolAllocator PlusTcpAddress_Allocator = {PlusTcpAddress_InUse, SOLIDSYSLOG_ADDRESS_POOL_SIZE};

struct SolidSyslogAddress* SolidSyslogPlusTcpAddress_Create(void)
{
    /* TU-private fallback returned when the pool is exhausted. Sized as
     * a real SolidSyslogPlusTcpAddress so a Resolver overwrite at the
     * exhausted-fallback call site is bounded — same freertos_sockaddr
     * storage as any pooled slot. Not a per-Sender slot: multi-overflow
     * integrators share this storage and race on it. Bumping
     * SOLIDSYSLOG_ADDRESS_POOL_SIZE removes the race. */
    static struct SolidSyslogPlusTcpAddress fallback;

    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&PlusTcpAddress_Allocator);
    struct SolidSyslogAddress* handle = (struct SolidSyslogAddress*) &fallback;
    if (SolidSyslogPoolAllocator_IndexIsValid(&PlusTcpAddress_Allocator, index) == true)
    {
        handle = PlusTcpAddress_HandleFromIndex(index);
        PlusTcpAddress_Initialise(handle);
    }
    else
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &PlusTcpAddressErrorSource,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            (int32_t) PLUSTCPADDRESS_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

static inline struct SolidSyslogAddress* PlusTcpAddress_HandleFromIndex(size_t index)
{
    static struct SolidSyslogPlusTcpAddress pool[SOLIDSYSLOG_ADDRESS_POOL_SIZE];
    return (struct SolidSyslogAddress*) &pool[index];
}

void SolidSyslogPlusTcpAddress_Destroy(struct SolidSyslogAddress* base)
{
    size_t index = PlusTcpAddress_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&PlusTcpAddress_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&PlusTcpAddress_Allocator, index, PlusTcpAddress_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &PlusTcpAddressErrorSource,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            (int32_t) PLUSTCPADDRESS_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t PlusTcpAddress_IndexFromHandle(const struct SolidSyslogAddress* base)
{
    size_t result = SOLIDSYSLOG_ADDRESS_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_ADDRESS_POOL_SIZE; poolIndex++)
    {
        if (base == PlusTcpAddress_HandleFromIndex(poolIndex))
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void PlusTcpAddress_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    PlusTcpAddress_Cleanup(PlusTcpAddress_HandleFromIndex(index));
}
