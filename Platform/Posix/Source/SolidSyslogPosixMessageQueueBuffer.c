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
    MAX_NAME_SIZE = 64,
    /* 0600 in octal — owner read/write, equivalent to S_IRUSR | S_IWUSR. Hex form avoids MISRA 7.1. */
    OWNER_READ_WRITE = 0x180U
};

static const char QUEUE_NAME_PREFIX[] = "/solidsyslog_";

static bool PosixMessageQueueBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
static void PosixMessageQueueBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size);

static inline struct SolidSyslogPosixMessageQueueBuffer* PosixMessageQueueBuffer_SelfFromBase(
    struct SolidSyslogBuffer* base
);

struct SolidSyslogPosixMessageQueueBuffer
{
    struct SolidSyslogBuffer Base;
    mqd_t Mq;
    SolidSyslogFormatterStorage NameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_NAME_SIZE)];
    size_t MaxMessageSize;
};

static struct SolidSyslogPosixMessageQueueBuffer instance;

static inline const char* PosixMessageQueueBuffer_QueueName(void)
{
    return SolidSyslogFormatter_AsFormattedBuffer(SolidSyslogFormatter_FromStorage(instance.NameStorage));
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- distinct semantic meaning; struct wrapper would over-engineer
struct SolidSyslogBuffer* SolidSyslogPosixMessageQueueBuffer_Create(size_t maxMessageSize, long maxMessages)
{
    instance = (struct SolidSyslogPosixMessageQueueBuffer) {0};

    struct SolidSyslogFormatter* name = SolidSyslogFormatter_Create(instance.NameStorage, MAX_NAME_SIZE);
    SolidSyslogFormatter_BoundedString(name, QUEUE_NAME_PREFIX, sizeof(QUEUE_NAME_PREFIX) - 1U);
    SolidSyslogPosixProcessId_Get(name);

    struct mq_attr attr = {0};
    attr.mq_maxmsg = maxMessages;
    attr.mq_msgsize = (long) maxMessageSize;

    instance.Mq = mq_open(PosixMessageQueueBuffer_QueueName(), O_CREAT | O_RDWR | O_NONBLOCK, OWNER_READ_WRITE, &attr);
    instance.MaxMessageSize = maxMessageSize;
    instance.Base.Write = PosixMessageQueueBuffer_Write;
    instance.Base.Read = PosixMessageQueueBuffer_Read;

    return &instance.Base;
}

void SolidSyslogPosixMessageQueueBuffer_Destroy(void)
{
    mq_close(instance.Mq);
    mq_unlink(PosixMessageQueueBuffer_QueueName());
    instance = (struct SolidSyslogPosixMessageQueueBuffer) {0};
}

static bool PosixMessageQueueBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead)
{
    struct SolidSyslogPosixMessageQueueBuffer* self = PosixMessageQueueBuffer_SelfFromBase(base);
    ssize_t received = mq_receive(self->Mq, data, maxSize, NULL);
    bool success = received >= 0;

    *bytesRead = success ? (size_t) received : 0U;

    return success;
}

static void PosixMessageQueueBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size)
{
    struct SolidSyslogPosixMessageQueueBuffer* self = PosixMessageQueueBuffer_SelfFromBase(base);
    mq_send(self->Mq, data, size, 0);
}

static inline struct SolidSyslogPosixMessageQueueBuffer* PosixMessageQueueBuffer_SelfFromBase(
    struct SolidSyslogBuffer* base
)
{
    return (struct SolidSyslogPosixMessageQueueBuffer*) base;
}
