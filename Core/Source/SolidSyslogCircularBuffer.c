#include "SolidSyslogCircularBuffer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogCircularBufferPrivate.h"
#include "SolidSyslogMutex.h"
#include "SolidSyslogTunables.h"

enum
{
    HEADER_BYTES = SOLIDSYSLOG_CIRCULAR_BUFFER_HEADER_BYTES
};

static bool CircularBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
static void CircularBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size);

static inline struct SolidSyslogCircularBuffer* CircularBuffer_SelfFromBase(struct SolidSyslogBuffer* base);

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
    struct SolidSyslogCircularBuffer* self,
    struct SolidSyslogMutex*          mutex,
    uint8_t*                          ring,
    size_t                            ringBytes
)
{
    self->Base.Read = CircularBuffer_Read;
    self->Base.Write = CircularBuffer_Write;
    self->Mutex = mutex;
    self->Ring = ring;
    self->Capacity = ringBytes;
    CircularBuffer_ResetToStart(self);
}

void CircularBuffer_Cleanup(struct SolidSyslogCircularBuffer* self)
{
    self->Base.Read = NULL;
    self->Base.Write = NULL;
    self->Mutex = NULL;
    self->Ring = NULL;
    self->Capacity = 0;
    self->Head = 0;
    self->Tail = 0;
    self->WrapPoint = 0;
}

static bool CircularBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead)
{
    struct SolidSyslogCircularBuffer* self = CircularBuffer_SelfFromBase(base);
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
            delivered = false;
        }
    }
    SolidSyslogMutex_Unlock(self->Mutex);
    return delivered;
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
    /* Little-endian read of the 2-byte length header out of the uint8_t ring. */
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
    /* Little-endian write of the 2-byte length header into the uint8_t ring. */
    self->Ring[self->Tail] = (uint8_t) (size & 0xFFU);
    self->Ring[self->Tail + 1U] = (uint8_t) ((size >> 8U) & 0xFFU);
    (void) memcpy(&self->Ring[self->Tail + HEADER_BYTES], data, size);
    self->Tail += HEADER_BYTES + size;
}
