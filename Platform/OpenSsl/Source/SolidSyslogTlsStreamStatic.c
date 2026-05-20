#include "SolidSyslogTlsStream.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTlsStreamPrivate.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogStream;

static inline size_t TlsStream_IndexFromHandle(const struct SolidSyslogStream* base);
static inline void TlsStream_CleanupAtIndex(size_t index, void* context);

static bool TlsStream_InUse[SOLIDSYSLOG_TLS_STREAM_POOL_SIZE];
static struct SolidSyslogTlsStream TlsStream_Pool[SOLIDSYSLOG_TLS_STREAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator TlsStream_Allocator = {TlsStream_InUse, SOLIDSYSLOG_TLS_STREAM_POOL_SIZE};

struct SolidSyslogStream* SolidSyslogTlsStream_Create(const struct SolidSyslogTlsStreamConfig* config)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&TlsStream_Allocator);
    struct SolidSyslogStream* handle = SolidSyslogNullStream_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&TlsStream_Allocator, index))
    {
        TlsStream_Initialise(&TlsStream_Pool[index].Base, config);
        handle = &TlsStream_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_TLSSTREAM_POOL_EXHAUSTED);
    }
    return handle;
}

void SolidSyslogTlsStream_Destroy(struct SolidSyslogStream* base)
{
    size_t index = TlsStream_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&TlsStream_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&TlsStream_Allocator, index, TlsStream_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_TLSSTREAM_UNKNOWN_DESTROY);
    }
}

static inline size_t TlsStream_IndexFromHandle(const struct SolidSyslogStream* base)
{
    size_t result = SOLIDSYSLOG_TLS_STREAM_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_TLS_STREAM_POOL_SIZE; poolIndex++)
    {
        if (base == &TlsStream_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void TlsStream_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    TlsStream_Cleanup(&TlsStream_Pool[index].Base);
}
