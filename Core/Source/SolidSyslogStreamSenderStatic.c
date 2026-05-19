#include "SolidSyslogStreamSender.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogNullSender.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamSenderPrivate.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogSender;

static size_t StreamSender_IndexFromHandle(const struct SolidSyslogSender* base);
static void StreamSender_CleanupAtIndex(size_t index, void* context);

static bool StreamSender_InUse[SOLIDSYSLOG_STREAM_SENDER_POOL_SIZE];
static struct SolidSyslogStreamSender StreamSender_Pool[SOLIDSYSLOG_STREAM_SENDER_POOL_SIZE];
static struct SolidSyslogPoolAllocator StreamSender_Allocator = {
    StreamSender_InUse,
    SOLIDSYSLOG_STREAM_SENDER_POOL_SIZE
};

struct SolidSyslogSender* SolidSyslogStreamSender_Create(const struct SolidSyslogStreamSenderConfig* config)
{
    struct SolidSyslogSender* result = SolidSyslogNullSender_Get();
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&StreamSender_Allocator);
    if (SolidSyslogPoolAllocator_IndexIsValid(&StreamSender_Allocator, index))
    {
        StreamSender_Initialise(&StreamSender_Pool[index].Base, config);
        result = &StreamSender_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_STREAMSENDER_POOL_EXHAUSTED);
    }
    return result;
}

void SolidSyslogStreamSender_Destroy(struct SolidSyslogSender* base)
{
    size_t index = StreamSender_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&StreamSender_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&StreamSender_Allocator, index, StreamSender_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_STREAMSENDER_UNKNOWN_DESTROY);
    }
}

static size_t StreamSender_IndexFromHandle(const struct SolidSyslogSender* base)
{
    size_t result = SOLIDSYSLOG_STREAM_SENDER_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_STREAM_SENDER_POOL_SIZE; poolIndex++)
    {
        if (base == &StreamSender_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static void StreamSender_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    StreamSender_Cleanup(&StreamSender_Pool[index].Base);
}
