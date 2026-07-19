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
#include "SolidSyslogMutex.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

const struct SolidSyslogErrorSource CircularBufferErrorSource = {"CircularBuffer"};

enum
{
    HEADER_BYTES = SOLIDSYSLOG_CIRCULAR_BUFFER_HEADER_BYTES
};

static bool CircularBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
static void CircularBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size);

static inline struct SolidSyslogCircularBuffer* CircularBuffer_SelfFromBase(struct SolidSyslogBuffer* base);

static inline void CircularBuffer_ReportRecordTooLarge(void);

static inline bool CircularBuffer_IsEmpty(const struct SolidSyslogCircularBuffer* self);
static inline bool CircularBuffer_IsWrapped(const struct SolidSyslogCircularBuffer* self);
static inline bool CircularBuffer_HeadAtWrapPoint(const struct SolidSyslogCircularBuffer* self);
static inline bool CircularBuffer_RecordFitsAtTail(const struct SolidSyslogCircularBuffer* self, size_t recordBytes);
static inline bool CircularBuffer_RecordFitsAfterWrap(const struct SolidSyslogCircularBuffer* self, size_t recordBytes);
static inline void CircularBuffer_ResetToStart(struct SolidSyslogCircularBuffer* self);
static inline void CircularBuffer_WrapTail(struct SolidSyslogCircularBuffer* self);
static inline void CircularBuffer_ConsumeWrapMarker(struct SolidSyslogCircularBuffer* self);
static inline void CircularBuffer_StoreRecord(struct SolidSyslogCircularBuffer* self, const void* data, size_t size);
static inline void CircularBuffer_LoadRecord(struct SolidSyslogCircularBuffer* self, void* data, size_t* bytesRead);
static inline size_t CircularBuffer_PeekRecordSize(const struct SolidSyslogCircularBuffer* self);

void CircularBuffer_Initialise(
    struct SolidSyslogBuffer* base,
    struct SolidSyslogMutex* mutex,
    uint8_t* ring,
    size_t ringBytes
)
{
    struct SolidSyslogCircularBuffer* self = CircularBuffer_SelfFromBase(base);
    self->Base.Read = CircularBuffer_Read;
    self->Base.Write = CircularBuffer_Write;
    self->Mutex = mutex;
    self->Ring = ring;
    self->Capacity = ringBytes;
    CircularBuffer_ResetToStart(self);
}

void CircularBuffer_Cleanup(struct SolidSyslogBuffer* base)
{
    /* Overwrite the abstract base with the shared NullBuffer vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. Derived
     * fields are private to this TU; the next _Initialise overwrites them. */
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
        CIRCULARBUFFER_ERROR_RECORD_TOO_LARGE
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
        SolidSyslogMutex_Lock(self->Mutex);
        if (CircularBuffer_IsEmpty(self))
        {
            CircularBuffer_ResetToStart(self);
        }
        size_t recordBytes = HEADER_BYTES + size;
        bool fitsAtTail = CircularBuffer_RecordFitsAtTail(self, recordBytes);
        bool shouldWrite = fitsAtTail || CircularBuffer_RecordFitsAfterWrap(self, recordBytes);
        if (shouldWrite)
        {
            if (!fitsAtTail)
            {
                CircularBuffer_WrapTail(self);
            }
            CircularBuffer_StoreRecord(self, data, size);
        }
        SolidSyslogMutex_Unlock(self->Mutex);
    }
}

static inline bool CircularBuffer_IsWrapped(const struct SolidSyslogCircularBuffer* self)
{
    return self->Head > self->Tail;
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
