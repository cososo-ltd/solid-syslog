#include "SolidSyslogPosixDatagram.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPosixDatagramPrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogDatagram;

static inline size_t PosixDatagram_IndexFromHandle(const struct SolidSyslogDatagram* base);
static inline void PosixDatagram_CleanupAtIndex(size_t index, void* context);

static bool PosixDatagram_InUse[SOLIDSYSLOG_POSIX_DATAGRAM_POOL_SIZE];
static struct SolidSyslogPosixDatagram PosixDatagram_Pool[SOLIDSYSLOG_POSIX_DATAGRAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator PosixDatagram_Allocator = {
    PosixDatagram_InUse,
    SOLIDSYSLOG_POSIX_DATAGRAM_POOL_SIZE
};

struct SolidSyslogDatagram* SolidSyslogPosixDatagram_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&PosixDatagram_Allocator);
    struct SolidSyslogDatagram* handle = SolidSyslogNullDatagram_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&PosixDatagram_Allocator, index) == true)
    {
        PosixDatagram_Initialise(&PosixDatagram_Pool[index].Base);
        handle = &PosixDatagram_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_POSIXDATAGRAM_POOL_EXHAUSTED);
    }
    return handle;
}

void SolidSyslogPosixDatagram_Destroy(struct SolidSyslogDatagram* base)
{
    size_t index = PosixDatagram_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&PosixDatagram_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&PosixDatagram_Allocator, index, PosixDatagram_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_POSIXDATAGRAM_UNKNOWN_DESTROY);
    }
}

static inline size_t PosixDatagram_IndexFromHandle(const struct SolidSyslogDatagram* base)
{
    size_t result = SOLIDSYSLOG_POSIX_DATAGRAM_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_POSIX_DATAGRAM_POOL_SIZE; poolIndex++)
    {
        if (base == &PosixDatagram_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void PosixDatagram_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    PosixDatagram_Cleanup(&PosixDatagram_Pool[index].Base);
}
