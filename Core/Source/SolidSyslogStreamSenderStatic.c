#include "SolidSyslogStreamSender.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullSender.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamSenderErrors.h"
#include "SolidSyslogStreamSenderPrivate.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogSender;

static bool StreamSender_IsValidConfig(const struct SolidSyslogStreamSenderConfig* config);
static inline size_t StreamSender_IndexFromHandle(const struct SolidSyslogSender* base);
static inline void StreamSender_CleanupAtIndex(size_t index, void* context);

static bool StreamSender_InUse[SOLIDSYSLOG_STREAM_SENDER_POOL_SIZE];
static struct SolidSyslogStreamSender StreamSender_Pool[SOLIDSYSLOG_STREAM_SENDER_POOL_SIZE];
static struct SolidSyslogPoolAllocator StreamSender_Allocator = {
    StreamSender_InUse,
    SOLIDSYSLOG_STREAM_SENDER_POOL_SIZE
};

struct SolidSyslogSender* SolidSyslogStreamSender_Create(const struct SolidSyslogStreamSenderConfig* config)
{
    struct SolidSyslogSender* result = SolidSyslogNullSender_Get();
    if (StreamSender_IsValidConfig(config))
    {
        size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&StreamSender_Allocator);
        if (SolidSyslogPoolAllocator_IndexIsValid(&StreamSender_Allocator, index))
        {
            StreamSender_Initialise(&StreamSender_Pool[index].Base, config);
            result = &StreamSender_Pool[index].Base;
        }
        else
        {
            SolidSyslog_Error(
                SOLIDSYSLOG_SEVERITY_ERROR,
                &StreamSenderErrorSource,
                SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
                (int32_t) STREAMSENDER_ERROR_POOL_EXHAUSTED
            );
        }
    }
    return result;
}

static bool StreamSender_IsValidConfig(const struct SolidSyslogStreamSenderConfig* config)
{
    bool valid = false;
    if (config == NULL)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &StreamSenderErrorSource,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            (int32_t) STREAMSENDER_ERROR_NULL_CONFIG
        );
    }
    else if (config->Resolver == NULL)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &StreamSenderErrorSource,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            (int32_t) STREAMSENDER_ERROR_NULL_RESOLVER
        );
    }
    else if (config->Stream == NULL)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &StreamSenderErrorSource,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            (int32_t) STREAMSENDER_ERROR_NULL_STREAM
        );
    }
    else if (config->Address == NULL)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &StreamSenderErrorSource,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            (int32_t) STREAMSENDER_ERROR_NULL_ADDRESS
        );
    }
    else
    {
        valid = true;
    }
    return valid;
}

void SolidSyslogStreamSender_Destroy(struct SolidSyslogSender* base)
{
    size_t index = StreamSender_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&StreamSender_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&StreamSender_Allocator, index, StreamSender_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &StreamSenderErrorSource,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            (int32_t) STREAMSENDER_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t StreamSender_IndexFromHandle(const struct SolidSyslogSender* base)
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

static inline void StreamSender_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    StreamSender_Cleanup(&StreamSender_Pool[index].Base);
}
