/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogCircularBuffer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogCircularBufferErrors.h"
#include "SolidSyslogCircularBufferPrivate.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogMutex.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

const struct SolidSyslogErrorSource SolidSyslogCircularBufferErrorSource = {"CircularBuffer"};

enum
{
    HEADER_BYTES = SOLIDSYSLOG_CIRCULAR_BUFFER_HEADER_BYTES
};

/* Which run of losses, if any, a write has just started. */
enum SolidSyslogCircularBufferRunStarted
{
    RUN_STARTED_NONE,
    RUN_STARTED_REFUSALS,
    RUN_STARTED_EVICTIONS
};

static bool CircularBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
static void CircularBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size);
static bool CircularBuffer_IsFull(struct SolidSyslogBuffer* base);

static inline struct SolidSyslogCircularBuffer* CircularBuffer_SelfFromBase(struct SolidSyslogBuffer* base);
static inline void CircularBuffer_InstallDiscardPolicy(
    struct SolidSyslogCircularBuffer* self,
    SolidSyslogDiscardPolicyFunction configured,
    void* context
);

static inline void CircularBuffer_ReportRecordTooLarge(void);
static enum SolidSyslogCircularBufferRunStarted CircularBuffer_StoreApplyingPolicy(
    struct SolidSyslogCircularBuffer* self,
    enum SolidSyslogDiscardPolicy policy,
    const void* data,
    size_t size
);
static inline bool CircularBuffer_PolicyEvictsFor(
    const struct SolidSyslogCircularBuffer* self,
    size_t recordBytes,
    enum SolidSyslogDiscardPolicy policy
);
static inline bool CircularBuffer_CouldHoldWhenEmpty(const struct SolidSyslogCircularBuffer* self, size_t recordBytes);
static inline void CircularBuffer_EvictUntilRecordFits(struct SolidSyslogCircularBuffer* self, size_t recordBytes);
static inline void CircularBuffer_DropHead(struct SolidSyslogCircularBuffer* self);
static inline enum SolidSyslogCircularBufferRunStarted CircularBuffer_KeepRecord(
    struct SolidSyslogCircularBuffer* self,
    const void* data,
    size_t size,
    bool evicted
);
static inline enum SolidSyslogCircularBufferRunStarted CircularBuffer_RefuseRecord(
    struct SolidSyslogCircularBuffer* self,
    size_t recordBytes
);
static void CircularBuffer_ReportRunStarted(enum SolidSyslogCircularBufferRunStarted started);
static inline void CircularBuffer_ReportRecordExceedsMaxMessageSize(void);

static inline bool CircularBuffer_IsEmpty(const struct SolidSyslogCircularBuffer* self);
static inline bool CircularBuffer_IsWrapped(const struct SolidSyslogCircularBuffer* self);
static inline bool CircularBuffer_HeadAtWrapPoint(const struct SolidSyslogCircularBuffer* self);
static inline bool CircularBuffer_RecordFits(const struct SolidSyslogCircularBuffer* self, size_t recordBytes);
static inline bool CircularBuffer_RecordFitsAtTail(const struct SolidSyslogCircularBuffer* self, size_t recordBytes);
static inline bool CircularBuffer_RecordFitsAfterWrap(const struct SolidSyslogCircularBuffer* self, size_t recordBytes);
static inline void CircularBuffer_ResetToStart(struct SolidSyslogCircularBuffer* self);
static inline void CircularBuffer_WrapTail(struct SolidSyslogCircularBuffer* self);
static inline void CircularBuffer_ConsumeWrapMarker(struct SolidSyslogCircularBuffer* self);
static inline void CircularBuffer_StoreRecord(struct SolidSyslogCircularBuffer* self, const void* data, size_t size);
static inline void CircularBuffer_LoadRecord(struct SolidSyslogCircularBuffer* self, void* data, size_t* bytesRead);
static inline size_t CircularBuffer_PeekRecordSize(const struct SolidSyslogCircularBuffer* self);

void SolidSyslogCircularBuffer_Initialise(
    struct SolidSyslogBuffer* base,
    struct SolidSyslogMutex* mutex,
    uint8_t* ring,
    size_t ringBytes,
    SolidSyslogDiscardPolicyFunction getDiscardPolicy,
    void* discardPolicyContext
)
{
    struct SolidSyslogCircularBuffer* self = CircularBuffer_SelfFromBase(base);
    self->Base.Read = CircularBuffer_Read;
    self->Base.Write = CircularBuffer_Write;
    self->Base.IsFull = CircularBuffer_IsFull;
    self->Mutex = mutex;
    self->Ring = ring;
    self->Capacity = ringBytes;
    CircularBuffer_InstallDiscardPolicy(self, getDiscardPolicy, discardPolicyContext);
    self->LossReported = false;
    self->EvictionReported = false;
    self->Full = false;
    CircularBuffer_ResetToStart(self);
}

static inline void CircularBuffer_InstallDiscardPolicy(
    struct SolidSyslogCircularBuffer* self,
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

void SolidSyslogCircularBuffer_Cleanup(struct SolidSyslogBuffer* base)
{
    /* Overwrite the abstract base with the shared NullBuffer vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. Derived
     * fields are private to this TU; the next SolidSyslogCircularBuffer_Initialise overwrites them. */
    *base = *SolidSyslogNullBuffer_Get();
}

static bool CircularBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead)
{
    struct SolidSyslogCircularBuffer* self = CircularBuffer_SelfFromBase(base);
    bool recordTooLarge = false;
    SolidSyslogMutex_Lock(self->Mutex);
    *bytesRead = 0;
    bool delivered = !CircularBuffer_IsEmpty(self);
    if (delivered)
    {
        if (CircularBuffer_HeadAtWrapPoint(self))
        {
            CircularBuffer_ConsumeWrapMarker(self);
        }
        if (CircularBuffer_PeekRecordSize(self) <= maxSize)
        {
            CircularBuffer_LoadRecord(self, data, bytesRead);
            self->Full = false;
        }
        else
        {
            recordTooLarge = true;
            delivered = false;
        }
    }
    SolidSyslogMutex_Unlock(self->Mutex);

    /* Reported after the unlock: the handler is integrator code and may log,
     * which re-enters Write and would deadlock on this same non-recursive mutex.
     * The oversized branch mutates no state, so deferring the report costs
     * nothing. */
    if (recordTooLarge)
    {
        CircularBuffer_ReportRecordTooLarge();
    }
    return delivered;
}

static inline void CircularBuffer_ReportRecordTooLarge(void)
{
    CircularBuffer_Report(
        SOLIDSYSLOG_SEVERITY_ERROR,
        SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED,
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORD_TOO_LARGE
    );
}

static inline struct SolidSyslogCircularBuffer* CircularBuffer_SelfFromBase(struct SolidSyslogBuffer* base)
{
    return (struct SolidSyslogCircularBuffer*) base;
}

static inline bool CircularBuffer_IsEmpty(const struct SolidSyslogCircularBuffer* self)
{
    return self->Head == self->Tail;
}

static inline bool CircularBuffer_HeadAtWrapPoint(const struct SolidSyslogCircularBuffer* self)
{
    return self->Head >= self->WrapPoint;
}

static inline void CircularBuffer_ConsumeWrapMarker(struct SolidSyslogCircularBuffer* self)
{
    self->Head = 0;
    self->WrapPoint = self->Capacity;
}

static inline size_t CircularBuffer_PeekRecordSize(const struct SolidSyslogCircularBuffer* self)
{
    return ((size_t) self->Ring[self->Head]) | (((size_t) self->Ring[self->Head + 1U]) << 8U);
}

static inline void CircularBuffer_LoadRecord(struct SolidSyslogCircularBuffer* self, void* data, size_t* bytesRead)
{
    size_t recordSize = CircularBuffer_PeekRecordSize(self);
    (void) memcpy(data, &self->Ring[self->Head + HEADER_BYTES], recordSize);
    self->Head += HEADER_BYTES + recordSize;
    *bytesRead = recordSize;
}

static void CircularBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size)
{
    struct SolidSyslogCircularBuffer* self = CircularBuffer_SelfFromBase(base);
    if (size <= SOLIDSYSLOG_MAX_MESSAGE_SIZE)
    {
        /* The policy is asked before the lock and the loss reported after it,
         * as Read reports: both are integrator code that may log, which
         * re-enters Write. */
        enum SolidSyslogDiscardPolicy policy = self->GetDiscardPolicy(self->DiscardPolicyContext);
        enum SolidSyslogCircularBufferRunStarted started = CircularBuffer_StoreApplyingPolicy(self, policy, data, size);
        CircularBuffer_ReportRunStarted(started);
    }
    else
    {
        CircularBuffer_ReportRecordExceedsMaxMessageSize();
    }
}

/* Stores the record, making room for it first where the discard policy says
 * to, and says which run of losses, if any, this write starts: the one write
 * of each run that is reported. A run is marked under the lock, so a write
 * from the handler reporting it finds it already reported. */
static enum SolidSyslogCircularBufferRunStarted CircularBuffer_StoreApplyingPolicy(
    struct SolidSyslogCircularBuffer* self,
    enum SolidSyslogDiscardPolicy policy,
    const void* data,
    size_t size
)
{
    enum SolidSyslogCircularBufferRunStarted started = RUN_STARTED_NONE;
    SolidSyslogMutex_Lock(self->Mutex);
    if (CircularBuffer_IsEmpty(self))
    {
        CircularBuffer_ResetToStart(self);
    }
    size_t recordBytes = HEADER_BYTES + size;
    bool evicted = false;
    if (!CircularBuffer_RecordFits(self, recordBytes) && CircularBuffer_PolicyEvictsFor(self, recordBytes, policy))
    {
        CircularBuffer_EvictUntilRecordFits(self, recordBytes);
        evicted = true;
    }
    if (CircularBuffer_RecordFits(self, recordBytes))
    {
        started = CircularBuffer_KeepRecord(self, data, size, evicted);
    }
    else
    {
        started = CircularBuffer_RefuseRecord(self, recordBytes);
    }
    SolidSyslogMutex_Unlock(self->Mutex);
    return started;
}

/* Only Oldest makes room, and only for a record the ring could hold at all:
 * evicting everything for one that still would not fit loses records for
 * nothing. */
static inline bool CircularBuffer_PolicyEvictsFor(
    const struct SolidSyslogCircularBuffer* self,
    size_t recordBytes,
    enum SolidSyslogDiscardPolicy policy
)
{
    return CircularBuffer_CouldHoldWhenEmpty(self, recordBytes) && (policy == SOLIDSYSLOG_DISCARD_POLICY_OLDEST);
}

static inline bool CircularBuffer_CouldHoldWhenEmpty(const struct SolidSyslogCircularBuffer* self, size_t recordBytes)
{
    return recordBytes <= self->Capacity;
}

/* Drops records from the head until the new one fits; an emptied ring always
 * fits a record it could hold at all, so this ends. */
static inline void CircularBuffer_EvictUntilRecordFits(struct SolidSyslogCircularBuffer* self, size_t recordBytes)
{
    while (!CircularBuffer_RecordFits(self, recordBytes))
    {
        CircularBuffer_DropHead(self);
    }
}

/* Skips the head record as Read would take it, without copying it out. */
static inline void CircularBuffer_DropHead(struct SolidSyslogCircularBuffer* self)
{
    if (CircularBuffer_HeadAtWrapPoint(self))
    {
        CircularBuffer_ConsumeWrapMarker(self);
    }
    self->Head += HEADER_BYTES + CircularBuffer_PeekRecordSize(self);
    if (CircularBuffer_IsEmpty(self))
    {
        CircularBuffer_ResetToStart(self);
    }
}

/* A record kept ends a run of refusals; one kept without evicting ends a run
 * of evictions. The ring is full afterwards only if it had to make room. */
static inline enum SolidSyslogCircularBufferRunStarted CircularBuffer_KeepRecord(
    struct SolidSyslogCircularBuffer* self,
    const void* data,
    size_t size,
    bool evicted
)
{
    enum SolidSyslogCircularBufferRunStarted started = RUN_STARTED_NONE;
    if (!CircularBuffer_RecordFitsAtTail(self, HEADER_BYTES + size))
    {
        CircularBuffer_WrapTail(self);
    }
    CircularBuffer_StoreRecord(self, data, size);
    self->LossReported = false;
    if (evicted && !self->EvictionReported)
    {
        started = RUN_STARTED_EVICTIONS;
    }
    self->EvictionReported = evicted;
    self->Full = evicted;
    return started;
}

/* A refusal evicts nothing, so it ends a run of evictions. It leaves the ring
 * full unless the record could never have fitted, which is a configuration
 * fault rather than Log outrunning Service. */
static inline enum SolidSyslogCircularBufferRunStarted CircularBuffer_RefuseRecord(
    struct SolidSyslogCircularBuffer* self,
    size_t recordBytes
)
{
    enum SolidSyslogCircularBufferRunStarted started = self->LossReported ? RUN_STARTED_NONE : RUN_STARTED_REFUSALS;
    self->LossReported = true;
    self->EvictionReported = false;
    self->Full = CircularBuffer_CouldHoldWhenEmpty(self, recordBytes);
    return started;
}

static void CircularBuffer_ReportRunStarted(enum SolidSyslogCircularBufferRunStarted started)
{
    if (started == RUN_STARTED_REFUSALS)
    {
        CircularBuffer_Report(
            SOLIDSYSLOG_SEVERITY_WARNING,
            SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
            SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORD_REFUSED
        );
    }
    else if (started == RUN_STARTED_EVICTIONS)
    {
        CircularBuffer_Report(
            SOLIDSYSLOG_SEVERITY_WARNING,
            SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
            SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORDS_EVICTED
        );
    }
    else
    {
        /* Kept without loss, or a run already reported. */
    }
}

static bool CircularBuffer_IsFull(struct SolidSyslogBuffer* base)
{
    struct SolidSyslogCircularBuffer* self = CircularBuffer_SelfFromBase(base);
    SolidSyslogMutex_Lock(self->Mutex);
    bool full = self->Full;
    SolidSyslogMutex_Unlock(self->Mutex);
    return full;
}

/* SolidSyslog_Log never formats a message this large, so only a caller writing
 * to the buffer directly can hand one over. */
static inline void CircularBuffer_ReportRecordExceedsMaxMessageSize(void)
{
    CircularBuffer_Report(
        SOLIDSYSLOG_BAD_ARGUMENT_SEVERITY,
        SOLIDSYSLOG_CAT_BAD_ARGUMENT,
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORD_EXCEEDS_MAX_MESSAGE_SIZE
    );
}

static inline bool CircularBuffer_IsWrapped(const struct SolidSyslogCircularBuffer* self)
{
    return self->Head > self->Tail;
}

static inline bool CircularBuffer_RecordFits(const struct SolidSyslogCircularBuffer* self, size_t recordBytes)
{
    return CircularBuffer_RecordFitsAtTail(self, recordBytes) || CircularBuffer_RecordFitsAfterWrap(self, recordBytes);
}

static inline bool CircularBuffer_RecordFitsAtTail(const struct SolidSyslogCircularBuffer* self, size_t recordBytes)
{
    bool fits = false;
    if (CircularBuffer_IsWrapped(self))
    {
        fits = (self->Tail + recordBytes) < self->Head;
    }
    else
    {
        fits = (self->Tail + recordBytes) <= self->Capacity;
    }
    return fits;
}

static inline bool CircularBuffer_RecordFitsAfterWrap(const struct SolidSyslogCircularBuffer* self, size_t recordBytes)
{
    return (!CircularBuffer_IsWrapped(self)) && (recordBytes < self->Head);
}

static inline void CircularBuffer_ResetToStart(struct SolidSyslogCircularBuffer* self)
{
    self->Head = 0;
    self->Tail = 0;
    self->WrapPoint = self->Capacity;
}

static inline void CircularBuffer_WrapTail(struct SolidSyslogCircularBuffer* self)
{
    self->WrapPoint = self->Tail;
    self->Tail = 0;
}

static inline void CircularBuffer_StoreRecord(struct SolidSyslogCircularBuffer* self, const void* data, size_t size)
{
    self->Ring[self->Tail] = (uint8_t) (size & 0xFFU);
    self->Ring[self->Tail + 1U] = (uint8_t) ((size >> 8U) & 0xFFU);
    (void) memcpy(&self->Ring[self->Tail + HEADER_BYTES], data, size);
    self->Tail += HEADER_BYTES + size;
}
