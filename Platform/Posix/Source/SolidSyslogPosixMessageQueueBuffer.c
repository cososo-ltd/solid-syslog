#include "SolidSyslogPosixMessageQueueBuffer.h"

#include <mqueue.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogPosixProcessId.h"

enum
{
    MAX_NAME_SIZE = 64
};

static const char QUEUE_NAME_PREFIX[] = "/solidsyslog_";

static bool Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead);
static void Write(struct SolidSyslogBuffer* self, const void* data, size_t size);

struct SolidSyslogPosixMessageQueueBuffer
{
    struct SolidSyslogBuffer    base;
    mqd_t                       mq;
    SolidSyslogFormatterStorage nameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_NAME_SIZE)];
    size_t                      maxMessageSize;
};

static struct SolidSyslogPosixMessageQueueBuffer instance;

static inline const char* QueueName(void)
{
    return SolidSyslogFormatter_AsFormattedBuffer(SolidSyslogFormatter_FromStorage(instance.nameStorage));
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- distinct semantic meaning; struct wrapper would over-engineer
struct SolidSyslogBuffer* SolidSyslogPosixMessageQueueBuffer_Create(size_t maxMessageSize, long maxMessages)
{
    instance = (struct SolidSyslogPosixMessageQueueBuffer) {0};

    struct SolidSyslogFormatter* name = SolidSyslogFormatter_Create(instance.nameStorage, MAX_NAME_SIZE);
    SolidSyslogFormatter_BoundedString(name, QUEUE_NAME_PREFIX, sizeof(QUEUE_NAME_PREFIX) - 1);
    SolidSyslogPosixProcessId_Get(name);

    struct mq_attr attr = {0};
    attr.mq_maxmsg      = maxMessages;
    attr.mq_msgsize     = (long) maxMessageSize;

    instance.mq             = mq_open(QueueName(), O_CREAT | O_RDWR | O_NONBLOCK, 0600, &attr);
    instance.maxMessageSize = maxMessageSize;
    instance.base.Write     = Write;
    instance.base.Read      = Read;

    return &instance.base;
}

void SolidSyslogPosixMessageQueueBuffer_Destroy(void)
{
    mq_close(instance.mq);
    mq_unlink(QueueName());
    instance = (struct SolidSyslogPosixMessageQueueBuffer) {0};
}

static bool Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead)
{
    struct SolidSyslogPosixMessageQueueBuffer* mqBuffer = (struct SolidSyslogPosixMessageQueueBuffer*) self;
    ssize_t                                    received = mq_receive(mqBuffer->mq, data, maxSize, NULL);
    bool                                       success  = received >= 0;

    *bytesRead = success ? (size_t) received : 0;

    return success;
}

static void Write(struct SolidSyslogBuffer* self, const void* data, size_t size)
{
    struct SolidSyslogPosixMessageQueueBuffer* mqBuffer = (struct SolidSyslogPosixMessageQueueBuffer*) self;
    mq_send(mqBuffer->mq, data, size, 0);
}
