/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPosixMessageQueueBuffer.h"

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogMutex.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogPosixMessageQueueBufferErrors.h"
#include "SolidSyslogPosixMessageQueueBufferPrivate.h"
#include "SolidSyslogPrival.h"

const struct SolidSyslogErrorSource SolidSyslogPosixMessageQueueBufferErrorSource = {"PosixMessageQueueBuffer"};

enum
{
    /* 0600 in octal - owner read/write, equivalent to S_IRUSR | S_IWUSR. Hex form avoids MISRA 7.1. */
    OWNER_READ_WRITE = 0x180U
};

/* Which run of losses, if any, a write has just started. */
enum SolidSyslogPosixMessageQueueBufferRunStarted
{
    RUN_STARTED_NONE,
    RUN_STARTED_REFUSALS,
    RUN_STARTED_FAILURES,
    RUN_STARTED_EVICTIONS
};

static bool PosixMessageQueueBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
static void PosixMessageQueueBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size);
static bool PosixMessageQueueBuffer_IsFull(struct SolidSyslogBuffer* base);
static inline void PosixMessageQueueBuffer_NoteRecordTaken(struct SolidSyslogPosixMessageQueueBuffer* self);
static enum SolidSyslogPosixMessageQueueBufferRunStarted PosixMessageQueueBuffer_SendNotingRuns(
    struct SolidSyslogPosixMessageQueueBuffer* self,
    const void* data,
    size_t size
);
static inline bool PosixMessageQueueBuffer_Send(
    struct SolidSyslogPosixMessageQueueBuffer* self,
    const void* data,
    size_t size,
    int* sendErrno
);
static inline bool PosixMessageQueueBuffer_PolicyEvicts(struct SolidSyslogPosixMessageQueueBuffer* self);
static inline bool PosixMessageQueueBuffer_EvictOne(struct SolidSyslogPosixMessageQueueBuffer* self);
static inline enum SolidSyslogPosixMessageQueueBufferRunStarted PosixMessageQueueBuffer_NoteSent(
    struct SolidSyslogPosixMessageQueueBuffer* self,
    bool evicted
);
static void PosixMessageQueueBuffer_ReportRunStarted(enum SolidSyslogPosixMessageQueueBufferRunStarted started);

static inline struct SolidSyslogPosixMessageQueueBuffer* PosixMessageQueueBuffer_SelfFromBase(
    struct SolidSyslogBuffer* base
);
static inline void PosixMessageQueueBuffer_InstallDiscardPolicy(
    struct SolidSyslogPosixMessageQueueBuffer* self,
    SolidSyslogDiscardPolicyFunction configured,
    void* context
);
static inline const char* PosixMessageQueueBuffer_QueueName(struct SolidSyslogPosixMessageQueueBuffer* self);

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- distinct semantic meaning; mirrors the public SolidSyslogPosixMessageQueueBuffer_Create signature plus a per-slot discriminator
bool SolidSyslogPosixMessageQueueBuffer_Initialise(
    struct SolidSyslogBuffer* base,
    struct SolidSyslogMutex* mutex,
    size_t maxMessageSize,
    size_t maxMessages,
    size_t slotIndex,
    SolidSyslogDiscardPolicyFunction getDiscardPolicy,
    void* discardPolicyContext
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
    SolidSyslogFormatter_Uint32(name, (uint32_t) getpid());
    SolidSyslogFormatter_AsciiCharacter(name, '_');
    SolidSyslogFormatter_Uint32(name, (uint32_t) slotIndex);

    struct mq_attr attr = {0};
    attr.mq_maxmsg = (long) maxMessages;
    attr.mq_msgsize = (long) maxMessageSize;

    self->Mq = mq_open(PosixMessageQueueBuffer_QueueName(self), O_CREAT | O_RDWR | O_NONBLOCK, OWNER_READ_WRITE, &attr);
    self->MaxMessageSize = maxMessageSize;
    self->Mutex = mutex;
    PosixMessageQueueBuffer_InstallDiscardPolicy(self, getDiscardPolicy, discardPolicyContext);
    self->RefusalReported = false;
    self->FailureReported = false;
    self->EvictionReported = false;
    self->Full = false;
    self->Base.Write = PosixMessageQueueBuffer_Write;
    self->Base.Read = PosixMessageQueueBuffer_Read;
    self->Base.IsFull = PosixMessageQueueBuffer_IsFull;
    return self->Mq != (mqd_t) -1;
}

static inline void PosixMessageQueueBuffer_InstallDiscardPolicy(
    struct SolidSyslogPosixMessageQueueBuffer* self,
    SolidSyslogDiscardPolicyFunction configured,
    void* context
)
{
    self->GetDiscardPolicy = SolidSyslogDiscardPolicy_Default;
    self->DiscardPolicyContext = NULL;
    if (configured != NULL)
    {
        self->GetDiscardPolicy = configured;
        self->DiscardPolicyContext = context;
    }
}

void SolidSyslogPosixMessageQueueBuffer_Cleanup(struct SolidSyslogBuffer* base)
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
         * EAGAIN is the empty-queue poll signal - part of the happy path and
         * must stay silent. Any other errno is a real failure worth surfacing. */
        int receiveErrno = success ? 0 : errno;
        if (success)
        {
            PosixMessageQueueBuffer_NoteRecordTaken(self);
        }
        else if (receiveErrno != EAGAIN)
        {
            PosixMessageQueueBuffer_Report(
                SOLIDSYSLOG_SEVERITY_ERROR,
                SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED,
                SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_RECEIVE_FAILED
            );
        }
        else
        {
            /* Empty: the poll's quiet answer. */
        }

        *bytesRead = success ? (size_t) received : 0U;
    }
    return success;
}

/* A record taken out makes room, so the queue is no longer full. */
static inline void PosixMessageQueueBuffer_NoteRecordTaken(struct SolidSyslogPosixMessageQueueBuffer* self)
{
    SolidSyslogMutex_Lock(self->Mutex);
    self->Full = false;
    SolidSyslogMutex_Unlock(self->Mutex);
}

static bool PosixMessageQueueBuffer_IsFull(struct SolidSyslogBuffer* base)
{
    struct SolidSyslogPosixMessageQueueBuffer* self = PosixMessageQueueBuffer_SelfFromBase(base);
    SolidSyslogMutex_Lock(self->Mutex);
    bool full = self->Full;
    SolidSyslogMutex_Unlock(self->Mutex);
    return full;
}

static void PosixMessageQueueBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size)
{
    struct SolidSyslogPosixMessageQueueBuffer* self = PosixMessageQueueBuffer_SelfFromBase(base);
    /* Tasks logging at once share the run state. The report is raised after
     * the unlock: the handler may log, which re-enters Write. */
    SolidSyslogMutex_Lock(self->Mutex);
    enum SolidSyslogPosixMessageQueueBufferRunStarted started =
        PosixMessageQueueBuffer_SendNotingRuns(self, data, size);
    SolidSyslogMutex_Unlock(self->Mutex);
    PosixMessageQueueBuffer_ReportRunStarted(started);
}

/* Sends the record, making room for it first where the discard policy says to,
 * and says which run of losses, if any, this write starts: the one write of
 * each run that is reported. A full queue, a queue that fails and a queue that
 * had to evict are separate runs, so a queue that fills and then breaks still
 * says it broke. The run is marked here, before it is reported, so a handler
 * that logs into the queue finds it already reported. */
static enum SolidSyslogPosixMessageQueueBufferRunStarted PosixMessageQueueBuffer_SendNotingRuns(
    struct SolidSyslogPosixMessageQueueBuffer* self,
    const void* data,
    size_t size
)
{
    enum SolidSyslogPosixMessageQueueBufferRunStarted started = RUN_STARTED_NONE;
    int sendErrno = 0;
    bool sent = PosixMessageQueueBuffer_Send(self, data, size, &sendErrno);
    bool evicted = false;

    if (!sent && (sendErrno == EAGAIN) && PosixMessageQueueBuffer_PolicyEvicts(self))
    {
        /* One record out, one send again: another task may have filled the
         * room meanwhile, and the refusal that follows reports that loss. */
        evicted = PosixMessageQueueBuffer_EvictOne(self);
        sent = evicted && PosixMessageQueueBuffer_Send(self, data, size, &sendErrno);
    }

    if (sent)
    {
        started = PosixMessageQueueBuffer_NoteSent(self, evicted);
    }
    else if (sendErrno == EAGAIN)
    {
        started = self->RefusalReported ? RUN_STARTED_NONE : RUN_STARTED_REFUSALS;
        self->RefusalReported = true;
        self->EvictionReported = false;
        self->Full = true;
    }
    else
    {
        started = self->FailureReported ? RUN_STARTED_NONE : RUN_STARTED_FAILURES;
        self->FailureReported = true;
        self->EvictionReported = false;
    }

    return started;
}

/* errno is captured straight after mq_send, as Read does, so the tests on it
 * read errno only after the call that set it (MISRA C:2012 Rule 22.10). */
static inline bool PosixMessageQueueBuffer_Send(
    struct SolidSyslogPosixMessageQueueBuffer* self,
    const void* data,
    size_t size,
    int* sendErrno
)
{
    bool sent = mq_send(self->Mq, data, size, 0) == 0;
    *sendErrno = sent ? 0 : errno;
    return sent;
}

/* Asked on the task that logs, at the moment the full queue has to decide. */
static inline bool PosixMessageQueueBuffer_PolicyEvicts(struct SolidSyslogPosixMessageQueueBuffer* self)
{
    return self->GetDiscardPolicy(self->DiscardPolicyContext) == SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
}

/* Takes the oldest record out of the queue and drops it. Fails when the queue
 * has emptied meanwhile, or holds a record larger than the scratch; either
 * way nothing was evicted and the send is not retried. */
static inline bool PosixMessageQueueBuffer_EvictOne(struct SolidSyslogPosixMessageQueueBuffer* self)
{
    return mq_receive(self->Mq, self->Evicted, sizeof(self->Evicted), NULL) >= 0;
}

/* A record sent ends the runs of refusals and failures; one sent without
 * evicting ends the run of evictions. The queue is full afterwards only if it
 * had to make room. */
static inline enum SolidSyslogPosixMessageQueueBufferRunStarted PosixMessageQueueBuffer_NoteSent(
    struct SolidSyslogPosixMessageQueueBuffer* self,
    bool evicted
)
{
    enum SolidSyslogPosixMessageQueueBufferRunStarted started = RUN_STARTED_NONE;
    self->RefusalReported = false;
    self->FailureReported = false;
    if (evicted && !self->EvictionReported)
    {
        started = RUN_STARTED_EVICTIONS;
    }
    self->EvictionReported = evicted;
    self->Full = evicted;
    return started;
}

/* A full queue is a record lost to capacity, refused or evicted; anything else
 * is the queue failing. */
static void PosixMessageQueueBuffer_ReportRunStarted(enum SolidSyslogPosixMessageQueueBufferRunStarted started)
{
    if (started == RUN_STARTED_REFUSALS)
    {
        PosixMessageQueueBuffer_Report(
            SOLIDSYSLOG_SEVERITY_WARNING,
            SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
            SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_RECORD_REFUSED
        );
    }
    else if (started == RUN_STARTED_EVICTIONS)
    {
        PosixMessageQueueBuffer_Report(
            SOLIDSYSLOG_SEVERITY_WARNING,
            SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
            SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_RECORDS_EVICTED
        );
    }
    else if (started == RUN_STARTED_FAILURES)
    {
        PosixMessageQueueBuffer_Report(
            SOLIDSYSLOG_SEVERITY_ERROR,
            SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED,
            SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_SEND_FAILED
        );
    }
    else
    {
        /* Sent, or a run already reported. */
    }
}

static inline struct SolidSyslogPosixMessageQueueBuffer* PosixMessageQueueBuffer_SelfFromBase(
    struct SolidSyslogBuffer* base
)
{
    return (struct SolidSyslogPosixMessageQueueBuffer*) base;
}
