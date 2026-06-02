#include "SolidSyslogWinsockAddress.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWinsockAddressErrors.h"
#include "SolidSyslogWinsockAddressPrivate.h"

struct SolidSyslogAddress;

static inline struct SolidSyslogAddress* WinsockAddress_HandleFromIndex(size_t index);
static inline size_t WinsockAddress_IndexFromHandle(const struct SolidSyslogAddress* base);
static inline void WinsockAddress_CleanupAtIndex(size_t index, void* context);

static bool WinsockAddress_InUse[SOLIDSYSLOG_ADDRESS_POOL_SIZE];
static struct SolidSyslogPoolAllocator WinsockAddress_Allocator = {WinsockAddress_InUse, SOLIDSYSLOG_ADDRESS_POOL_SIZE};

struct SolidSyslogAddress* SolidSyslogWinsockAddress_Create(void)
{
    /* TU-private fallback returned when the pool is exhausted. Sized as
     * a real SolidSyslogWinsockAddress so a Resolver overwrite at the
     * exhausted-fallback call site is bounded — same sockaddr_in storage
     * as any pooled slot. Not a per-Sender slot: multi-overflow integrators
     * share this storage and race on it. Bumping SOLIDSYSLOG_ADDRESS_POOL_SIZE
     * removes the race. */
    static struct SolidSyslogWinsockAddress fallback;

    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&WinsockAddress_Allocator);
    struct SolidSyslogAddress* handle = (struct SolidSyslogAddress*) &fallback;
    if (SolidSyslogPoolAllocator_IndexIsValid(&WinsockAddress_Allocator, index) == true)
    {
        handle = WinsockAddress_HandleFromIndex(index);
        WinsockAddress_Initialise(handle);
    }
    else
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &WinsockAddressErrorSource,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            (int32_t) WINSOCKADDRESS_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

static inline struct SolidSyslogAddress* WinsockAddress_HandleFromIndex(size_t index)
{
    static struct SolidSyslogWinsockAddress pool[SOLIDSYSLOG_ADDRESS_POOL_SIZE];
    return (struct SolidSyslogAddress*) &pool[index];
}

void SolidSyslogWinsockAddress_Destroy(struct SolidSyslogAddress* base)
{
    size_t index = WinsockAddress_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&WinsockAddress_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&WinsockAddress_Allocator, index, WinsockAddress_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &WinsockAddressErrorSource,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            (int32_t) WINSOCKADDRESS_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t WinsockAddress_IndexFromHandle(const struct SolidSyslogAddress* base)
{
    size_t result = SOLIDSYSLOG_ADDRESS_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_ADDRESS_POOL_SIZE; poolIndex++)
    {
        if (base == WinsockAddress_HandleFromIndex(poolIndex))
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void WinsockAddress_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    WinsockAddress_Cleanup(WinsockAddress_HandleFromIndex(index));
}
