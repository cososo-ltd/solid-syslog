#include "SolidSyslogCircularBuffer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogMacros.h"
#include "SolidSyslogMutex.h"
#include "SolidSyslogTunables.h"

enum
{
    HEADER_BYTES = SOLIDSYSLOG_CIRCULARBUFFER_HEADER_BYTES
};

struct SolidSyslogCircularBuffer
{
    struct SolidSyslogBuffer Base;
    struct SolidSyslogMutex* Mutex;
    size_t Capacity;
    size_t Head;
    size_t Tail;
    size_t WrapPoint;
    uint8_t Storage[];
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(struct SolidSyslogCircularBuffer) ==
        (SOLIDSYSLOG_CIRCULARBUFFER_OVERHEAD * sizeof(SolidSyslogCircularBufferStorage)),
    "SOLIDSYSLOG_CIRCULARBUFFER_OVERHEAD does not match struct layout"
);

static bool CircularBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
static void CircularBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size);

static inline struct SolidSyslogCircularBuffer* CircularBuffer_SelfFromStorage(SolidSyslogCircularBufferStorage* storage
);
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

struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(
    SolidSyslogCircularBufferStorage* storage,
    size_t storageBytes,
    struct SolidSyslogMutex* mutex
)
{
    struct SolidSyslogCircularBuffer* self = CircularBuffer_SelfFromStorage(storage);
    self->Base.Read = CircularBuffer_Read;
    self->Base.Write = CircularBuffer_Write;
    self->Mutex = mutex;
    self->Capacity = storageBytes - sizeof(struct SolidSyslogCircularBuffer);
    CircularBuffer_ResetToStart(self);
    return &self->Base;
}

static inline struct SolidSyslogCircularBuffer* CircularBuffer_SelfFromStorage(SolidSyslogCircularBufferStorage* storage
)
{
    return (struct SolidSyslogCircularBuffer*) storage;
}

void SolidSyslogCircularBuffer_Destroy(struct SolidSyslogBuffer* base)
{
    struct SolidSyslogCircularBuffer* self = CircularBuffer_SelfFromBase(base);
    self->Base.Read = NULL;
    self->Base.Write = NULL;
    self->Mutex = NULL;
    self->Capacity = 0;
    self->Head = 0;
    self->Tail = 0;
    self->WrapPoint = 0;
}

static inline struct SolidSyslogCircularBuffer* CircularBuffer_SelfFromBase(struct SolidSyslogBuffer* base)
{
    return (struct SolidSyslogCircularBuffer*) base;
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
    uint16_t header = 0;
    memcpy(&header, &self->Storage[self->Head], HEADER_BYTES);
    return header;
}

static inline void CircularBuffer_LoadRecord(struct SolidSyslogCircularBuffer* self, void* data, size_t* bytesRead)
{
    size_t recordSize = CircularBuffer_PeekRecordSize(self);
    memcpy(data, &self->Storage[self->Head + HEADER_BYTES], recordSize);
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
    if (CircularBuffer_IsWrapped(self))
    {
        return (self->Tail + recordBytes) < self->Head;
    }
    return (self->Tail + recordBytes) <= self->Capacity;
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
    uint16_t header = (uint16_t) size;
    memcpy(&self->Storage[self->Tail], &header, HEADER_BYTES);
    memcpy(&self->Storage[self->Tail + HEADER_BYTES], data, size);
    self->Tail += HEADER_BYTES + size;
}
