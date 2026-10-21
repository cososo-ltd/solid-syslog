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
#include "SolidSyslogTunables.h"

const struct SolidSyslogErrorSource SolidSyslogPosixMessageQueueBufferErrorSource = {
    "PosixMessageQueueBuffer",
    SolidSyslogPosixMessageQueueBufferErrors_Name
};

enum
{
    /* 0600 in octal - owner read/write, equivalent to S_IRUSR | S_IWUSR. Hex form avoids MISRA 7.1. */
    OWNER_READ_WRITE = 0x180U
};

/* Which run of sends the queue would not take, if any, a write has just
 * started. A run of evictions is noted apart, since a write can evict and
 * still be refused. */
enum SolidSyslogPosixMessageQueueBufferRunStarted
{
    RUN_STARTED_NONE,
    RUN_STARTED_REFUSALS,
    RUN_STARTED_FAILURES
};

static bool PosixMessageQueueBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
static void PosixMessageQueueBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size);
static bool PosixMessageQueueBuffer_IsFull(struct SolidSyslogBuffer* base);
static inline void PosixMessageQueueBuffer_NoteRecordTaken(struct SolidSyslogPosixMessageQueueBuffer* self);
static enum SolidSyslogPosixMessageQueueBufferRunStarted PosixMessageQueueBuffer_SendNotingRuns(
    struct SolidSyslogPosixMessageQueueBuffer* self,
    enum SolidSyslogDiscardPolicy policy,
    const void* data,
    size_t size,
    bool* evictionRunStarted
);
static inline bool PosixMessageQueueBuffer_Send(
    struct SolidSyslogPosixMessageQueueBuffer* self,
    const void* data,
    size_t size,
    int* sendErrno
);
static inline bool PosixMessageQueueBuffer_EvictOne(const struct SolidSyslogPosixMessageQueueBuffer* self);
static inline bool PosixMessageQueueBuffer_NoteEvicted(struct SolidSyslogPosixMessageQueueBuffer* self, bool evicted);
static inline void PosixMessageQueueBuffer_ReportRecordsEvicted(void);
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
    /* Tasks logging at once share the run state. The policy is asked before
     * the lock and the losses reported after it: both are integrator code that
     * may log, which re-enters Write. */
    enum SolidSyslogDiscardPolicy policy = self->GetDiscardPolicy(self->DiscardPolicyContext);
    bool evictionRunStarted = false;
    SolidSyslogMutex_Lock(self->Mutex);
    enum SolidSyslogPosixMessageQueueBufferRunStarted started =
        PosixMessageQueueBuffer_SendNotingRuns(self, policy, data, size, &evictionRunStarted);
    SolidSyslogMutex_Unlock(self->Mutex);
    if (evictionRunStarted)
    {
        PosixMessageQueueBuffer_ReportRecordsEvicted();
    }
    PosixMessageQueueBuffer_ReportRunStarted(started);
}

/* Sends the record, making room for it first where the discard policy says to,
 * and says which run of losses, if any, this write starts: the one write of
 * each run that is reported. A full queue, a queue that fails and a queue that
 * had to evict are separate runs, so a queue that fills and then breaks still
 * says it broke, and one that evicts and is still refused - another task filled
 * the room - says both. The runs are marked here, before they are reported, so
 * a handler that logs into the queue finds them already reported. */
static enum SolidSyslogPosixMessageQueueBufferRunStarted PosixMessageQueueBuffer_SendNotingRuns(
    struct SolidSyslogPosixMessageQueueBuffer* self,
    enum SolidSyslogDiscardPolicy policy,
    const void* data,
    size_t size,
    bool* evictionRunStarted
)
{
    enum SolidSyslogPosixMessageQueueBufferRunStarted started = RUN_STARTED_NONE;
    int sendErrno = 0;
    bool sent = PosixMessageQueueBuffer_Send(self, data, size, &sendErrno);
    bool evicted = false;

    if (!sent && (sendErrno == EAGAIN) && (policy == SOLIDSYSLOG_DISCARD_POLICY_OLDEST))
    {
        evicted = PosixMessageQueueBuffer_EvictOne(self);
        sent = evicted && PosixMessageQueueBuffer_Send(self, data, size, &sendErrno);
    }
    *evictionRunStarted = PosixMessageQueueBuffer_NoteEvicted(self, evicted);

    if (sent)
    {
        self->RefusalReported = false;
        self->FailureReported = false;
        self->Full = evicted;
    }
    else if (sendErrno == EAGAIN)
    {
        started = self->RefusalReported ? RUN_STARTED_NONE : RUN_STARTED_REFUSALS;
        self->RefusalReported = true;
        self->Full = true;
    }
    else
    {
        started = self->FailureReported ? RUN_STARTED_NONE : RUN_STARTED_FAILURES;
        self->FailureReported = true;
        self->Full = self->Full || evicted;
    }

    return started;
}

/* A send that evicted continues or starts the run of evictions; one that
 * evicted nothing ends it. Says whether a run started. */
static inline bool PosixMessageQueueBuffer_NoteEvicted(struct SolidSyslogPosixMessageQueueBuffer* self, bool evicted)
{
    bool runStarted = evicted && !self->EvictionReported;
    self->EvictionReported = evicted;
    return runStarted;
}

static inline void PosixMessageQueueBuffer_ReportRecordsEvicted(void)
{
    PosixMessageQueueBuffer_Report(
        SOLIDSYSLOG_SEVERITY_WARNING,
        SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_RECORDS_EVICTED
    );
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

/* Takes the oldest record out of the queue and drops it. Fails when the queue
 * has emptied meanwhile, or holds a record larger than the scratch; either
 * way nothing was evicted and the send is not retried. */
static inline bool PosixMessageQueueBuffer_EvictOne(const struct SolidSyslogPosixMessageQueueBuffer* self)
{
    char evicted[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    return mq_receive(self->Mq, evicted, sizeof(evicted), NULL) >= 0;
}

/* A full queue is a record lost to capacity; anything else is the queue
 * failing. */
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
