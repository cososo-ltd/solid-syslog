#include "SolidSyslogPosixTcpStream.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPosixTcpStreamPrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogStream;

static inline size_t PosixTcpStream_IndexFromHandle(const struct SolidSyslogStream* base);
static inline void PosixTcpStream_CleanupAtIndex(size_t index, void* context);

static bool PosixTcpStream_InUse[SOLIDSYSLOG_POSIX_TCP_STREAM_POOL_SIZE];
static struct SolidSyslogPosixTcpStream PosixTcpStream_Pool[SOLIDSYSLOG_POSIX_TCP_STREAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator PosixTcpStream_Allocator = {
    PosixTcpStream_InUse,
    SOLIDSYSLOG_POSIX_TCP_STREAM_POOL_SIZE
};

struct SolidSyslogStream* SolidSyslogPosixTcpStream_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&PosixTcpStream_Allocator);
    struct SolidSyslogStream* handle = SolidSyslogNullStream_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&PosixTcpStream_Allocator, index) == true)
    {
        PosixTcpStream_Initialise(&PosixTcpStream_Pool[index].Base);
        handle = &PosixTcpStream_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_POSIXTCPSTREAM_POOL_EXHAUSTED);
    }
    return handle;
}

void SolidSyslogPosixTcpStream_Destroy(struct SolidSyslogStream* base)
{
    size_t index = PosixTcpStream_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&PosixTcpStream_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&PosixTcpStream_Allocator, index, PosixTcpStream_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_POSIXTCPSTREAM_UNKNOWN_DESTROY);
    }
}

static inline size_t PosixTcpStream_IndexFromHandle(const struct SolidSyslogStream* base)
{
    size_t result = SOLIDSYSLOG_POSIX_TCP_STREAM_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_POSIX_TCP_STREAM_POOL_SIZE; poolIndex++)
    {
        if (base == &PosixTcpStream_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void PosixTcpStream_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    PosixTcpStream_Cleanup(&PosixTcpStream_Pool[index].Base);
}
