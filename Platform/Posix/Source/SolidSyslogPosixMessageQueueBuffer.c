#include "SolidSyslogPosixMessageQueueBuffer.h"

#include <fcntl.h>
#include <mqueue.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogPosixMessageQueueBufferPrivate.h"
#include "SolidSyslogPosixProcessId.h"

enum
{
    /* 0600 in octal — owner read/write, equivalent to S_IRUSR | S_IWUSR. Hex form avoids MISRA 7.1. */
    OWNER_READ_WRITE = 0x180U
};

static bool PosixMessageQueueBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
static void PosixMessageQueueBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size);

static inline struct SolidSyslogPosixMessageQueueBuffer* PosixMessageQueueBuffer_SelfFromBase(
    struct SolidSyslogBuffer* base
);
static inline const char* PosixMessageQueueBuffer_QueueName(struct SolidSyslogPosixMessageQueueBuffer* self);

void PosixMessageQueueBuffer_Initialise(struct SolidSyslogBuffer* base, size_t maxMessageSize, long maxMessages)
{
    static const char queueNamePrefix[] = "/solidsyslog_";

    struct SolidSyslogPosixMessageQueueBuffer* self = PosixMessageQueueBuffer_SelfFromBase(base);

    struct SolidSyslogFormatter* name =
        SolidSyslogFormatter_Create(self->NameStorage, POSIX_MESSAGE_QUEUE_BUFFER_MAX_NAME_SIZE);
    SolidSyslogFormatter_BoundedString(name, queueNamePrefix, sizeof(queueNamePrefix) - 1U);
    SolidSyslogPosixProcessId_Get(name);

    struct mq_attr attr = {0};
    attr.mq_maxmsg = maxMessages;
    attr.mq_msgsize = (long) maxMessageSize;

    self->Mq = mq_open(PosixMessageQueueBuffer_QueueName(self), O_CREAT | O_RDWR | O_NONBLOCK, OWNER_READ_WRITE, &attr);
    self->MaxMessageSize = maxMessageSize;
    self->Base.Write = PosixMessageQueueBuffer_Write;
    self->Base.Read = PosixMessageQueueBuffer_Read;
}

void PosixMessageQueueBuffer_Cleanup(struct SolidSyslogBuffer* base)
{
    struct SolidSyslogPosixMessageQueueBuffer* self = PosixMessageQueueBuffer_SelfFromBase(base);
    mq_close(self->Mq);
    mq_unlink(PosixMessageQueueBuffer_QueueName(self));
    /* Overwrite the abstract base with the shared NullBuffer vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullBuffer_Get();
}

static inline const char* PosixMessageQueueBuffer_QueueName(struct SolidSyslogPosixMessageQueueBuffer* self)
{
    return SolidSyslogFormatter_AsFormattedBuffer(SolidSyslogFormatter_FromStorage(self->NameStorage));
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
