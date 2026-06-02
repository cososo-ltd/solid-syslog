#include "SolidSyslogPosixMessageQueueBuffer.h"

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogPosixMessageQueueBufferErrors.h"
#include "SolidSyslogPosixMessageQueueBufferPrivate.h"
#include "SolidSyslogPosixProcessId.h"
#include "SolidSyslogPrival.h"

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

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- distinct semantic meaning; mirrors the public _Create signature plus a per-slot discriminator
bool PosixMessageQueueBuffer_Initialise(
    struct SolidSyslogBuffer* base,
    size_t maxMessageSize,
    long maxMessages,
    size_t slotIndex
)
// NOLINTEND(bugprone-easily-swappable-parameters)
{
    static const char queueNamePrefix[] = "/solidsyslog_";

    struct SolidSyslogPosixMessageQueueBuffer* self = PosixMessageQueueBuffer_SelfFromBase(base);

    /* Queue name: /solidsyslog_<pid>_<slotIndex>. The pid keeps the name
     * unique per process; the slot index keeps multiple in-process pool
     * entries from aliasing onto the same kernel queue object. */
    struct SolidSyslogFormatter* name =
        SolidSyslogFormatter_Create(self->NameStorage, POSIX_MESSAGE_QUEUE_BUFFER_MAX_NAME_SIZE);
    SolidSyslogFormatter_BoundedString(name, queueNamePrefix, sizeof(queueNamePrefix) - 1U);
    SolidSyslogPosixProcessId_Get(name);
    SolidSyslogFormatter_AsciiCharacter(name, '_');
    SolidSyslogFormatter_Uint32(name, (uint32_t) slotIndex);

    struct mq_attr attr = {0};
    attr.mq_maxmsg = maxMessages;
    attr.mq_msgsize = (long) maxMessageSize;

    self->Mq = mq_open(PosixMessageQueueBuffer_QueueName(self), O_CREAT | O_RDWR | O_NONBLOCK, OWNER_READ_WRITE, &attr);
    self->MaxMessageSize = maxMessageSize;
    self->Base.Write = PosixMessageQueueBuffer_Write;
    self->Base.Read = PosixMessageQueueBuffer_Read;
    return self->Mq != (mqd_t) -1;
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
    bool success = false;
    if (bytesRead != NULL)
    {
        struct SolidSyslogPosixMessageQueueBuffer* self = PosixMessageQueueBuffer_SelfFromBase(base);
        ssize_t received = mq_receive(self->Mq, data, maxSize, NULL);
        success = received >= 0;

        /* Capture errno immediately after mq_receive so the EAGAIN test below
         * stays a pure predicate and is decoupled from errno's lifetime
         * between the errno-setting call and the read (MISRA C:2012 Rule 22.10).
         * EAGAIN is the empty-queue poll signal — part of the happy path and
         * must stay silent. Any other errno is a real failure worth surfacing. */
        int receiveErrno = success ? 0 : errno;
        if (!success && (receiveErrno != EAGAIN))
        {
            SolidSyslog_Error(
                SOLIDSYSLOG_SEVERITY_ERROR,
                &PosixMessageQueueBufferErrorSource,
                SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED,
                (int32_t) POSIXMESSAGEQUEUEBUFFER_ERROR_RECEIVE_FAILED
            );
        }

        *bytesRead = success ? (size_t) received : 0U;
    }
    return success;
}

static void PosixMessageQueueBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size)
{
    struct SolidSyslogPosixMessageQueueBuffer* self = PosixMessageQueueBuffer_SelfFromBase(base);
    if (mq_send(self->Mq, data, size, 0) != 0)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &PosixMessageQueueBufferErrorSource,
            SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED,
            (int32_t) POSIXMESSAGEQUEUEBUFFER_ERROR_SEND_FAILED
        );
    }
}

static inline struct SolidSyslogPosixMessageQueueBuffer* PosixMessageQueueBuffer_SelfFromBase(
    struct SolidSyslogBuffer* base
)
{
    return (struct SolidSyslogPosixMessageQueueBuffer*) base;
}
