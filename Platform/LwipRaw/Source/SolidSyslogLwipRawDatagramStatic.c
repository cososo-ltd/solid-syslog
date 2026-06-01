#include "SolidSyslogLwipRawDatagram.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipRawDatagramErrors.h"
#include "SolidSyslogLwipRawDatagramPrivate.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogDatagram;

static inline size_t LwipRawDatagram_IndexFromHandle(const struct SolidSyslogDatagram* base);
static inline void LwipRawDatagram_CleanupAtIndex(size_t index, void* context);

static bool LwipRawDatagram_InUse[SOLIDSYSLOG_DATAGRAM_POOL_SIZE];
static struct SolidSyslogLwipRawDatagram LwipRawDatagram_Pool[SOLIDSYSLOG_DATAGRAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator LwipRawDatagram_Allocator = {
    LwipRawDatagram_InUse,
    SOLIDSYSLOG_DATAGRAM_POOL_SIZE
};

struct SolidSyslogDatagram* SolidSyslogLwipRawDatagram_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&LwipRawDatagram_Allocator);
    struct SolidSyslogDatagram* handle = SolidSyslogNullDatagram_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&LwipRawDatagram_Allocator, index) == true)
    {
        LwipRawDatagram_Initialise(&LwipRawDatagram_Pool[index].Base);
        handle = &LwipRawDatagram_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &LwipRawDatagramErrorSource,
            (uint8_t) LWIPRAWDATAGRAM_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogLwipRawDatagram_Destroy(struct SolidSyslogDatagram* base)
{
    size_t index = LwipRawDatagram_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&LwipRawDatagram_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&LwipRawDatagram_Allocator, index, LwipRawDatagram_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &LwipRawDatagramErrorSource,
            (uint8_t) LWIPRAWDATAGRAM_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t LwipRawDatagram_IndexFromHandle(const struct SolidSyslogDatagram* base)
{
    size_t result = SOLIDSYSLOG_DATAGRAM_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_DATAGRAM_POOL_SIZE; poolIndex++)
    {
        if (base == &LwipRawDatagram_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void LwipRawDatagram_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    LwipRawDatagram_Cleanup(&LwipRawDatagram_Pool[index].Base);
}
