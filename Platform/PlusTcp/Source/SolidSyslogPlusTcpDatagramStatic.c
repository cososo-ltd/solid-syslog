#include "SolidSyslogPlusTcpDatagram.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogPlusTcpDatagramErrors.h"
#include "SolidSyslogPlusTcpDatagramPrivate.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogDatagram;

static inline size_t PlusTcpDatagram_IndexFromHandle(const struct SolidSyslogDatagram* base);
static inline void PlusTcpDatagram_CleanupAtIndex(size_t index, void* context);

static bool PlusTcpDatagram_InUse[SOLIDSYSLOG_DATAGRAM_POOL_SIZE];
static struct SolidSyslogPlusTcpDatagram PlusTcpDatagram_Pool[SOLIDSYSLOG_DATAGRAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator PlusTcpDatagram_Allocator = {
    PlusTcpDatagram_InUse,
    SOLIDSYSLOG_DATAGRAM_POOL_SIZE
};

struct SolidSyslogDatagram* SolidSyslogPlusTcpDatagram_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&PlusTcpDatagram_Allocator);
    struct SolidSyslogDatagram* handle = SolidSyslogNullDatagram_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&PlusTcpDatagram_Allocator, index) == true)
    {
        PlusTcpDatagram_Initialise(&PlusTcpDatagram_Pool[index].Base);
        handle = &PlusTcpDatagram_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &PlusTcpDatagramErrorSource,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            (int32_t) PLUSTCPDATAGRAM_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogPlusTcpDatagram_Destroy(struct SolidSyslogDatagram* base)
{
    size_t index = PlusTcpDatagram_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&PlusTcpDatagram_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&PlusTcpDatagram_Allocator, index, PlusTcpDatagram_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &PlusTcpDatagramErrorSource,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            (int32_t) PLUSTCPDATAGRAM_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t PlusTcpDatagram_IndexFromHandle(const struct SolidSyslogDatagram* base)
{
    size_t result = SOLIDSYSLOG_DATAGRAM_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_DATAGRAM_POOL_SIZE; poolIndex++)
    {
        if (base == &PlusTcpDatagram_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void PlusTcpDatagram_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    PlusTcpDatagram_Cleanup(&PlusTcpDatagram_Pool[index].Base);
}
