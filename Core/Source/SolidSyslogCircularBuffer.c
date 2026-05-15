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
    struct SolidSyslogBuffer base;
    struct SolidSyslogMutex* mutex;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t wrapPoint;
    uint8_t storage[];
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(struct SolidSyslogCircularBuffer) ==
        SOLIDSYSLOG_CIRCULARBUFFER_OVERHEAD * sizeof(SolidSyslogCircularBufferStorage),
    "SOLIDSYSLOG_CIRCULARBUFFER_OVERHEAD does not match struct layout"
);

static bool CircularBuffer_Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead);
static void CircularBuffer_Write(struct SolidSyslogBuffer* self, const void* data, size_t size);

static inline bool CircularBuffer_IsEmpty(const struct SolidSyslogCircularBuffer* circular);
static inline bool CircularBuffer_IsWrapped(const struct SolidSyslogCircularBuffer* circular);
static inline bool CircularBuffer_HeadAtWrapPoint(const struct SolidSyslogCircularBuffer* circular);
static inline bool CircularBuffer_RecordFitsAtTail(
    const struct SolidSyslogCircularBuffer* circular,
    size_t recordBytes
);
static inline bool CircularBuffer_RecordFitsAfterWrap(
    const struct SolidSyslogCircularBuffer* circular,
    size_t recordBytes
);
static inline void CircularBuffer_ResetToStart(struct SolidSyslogCircularBuffer* circular);
static inline void CircularBuffer_WrapTail(struct SolidSyslogCircularBuffer* circular);
static inline void CircularBuffer_ConsumeWrapMarker(struct SolidSyslogCircularBuffer* circular);
static inline void CircularBuffer_StoreRecord(
    struct SolidSyslogCircularBuffer* circular,
    const void* data,
    size_t size
);
static inline void CircularBuffer_LoadRecord(struct SolidSyslogCircularBuffer* circular, void* data, size_t* bytesRead);
static inline size_t CircularBuffer_PeekRecordSize(const struct SolidSyslogCircularBuffer* circular);

struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(
    SolidSyslogCircularBufferStorage* storage,
    size_t storageBytes,
    struct SolidSyslogMutex* mutex
)
{
    struct SolidSyslogCircularBuffer* circular = (struct SolidSyslogCircularBuffer*) storage;
    circular->base.Read = CircularBuffer_Read;
    circular->base.Write = CircularBuffer_Write;
    circular->mutex = mutex;
    circular->capacity = storageBytes - sizeof(struct SolidSyslogCircularBuffer);
    CircularBuffer_ResetToStart(circular);
    return &circular->base;
}

void SolidSyslogCircularBuffer_Destroy(struct SolidSyslogBuffer* buffer)
{
    struct SolidSyslogCircularBuffer* circular = (struct SolidSyslogCircularBuffer*) buffer;
    circular->base.Read = NULL;
    circular->base.Write = NULL;
    circular->mutex = NULL;
    circular->capacity = 0;
    circular->head = 0;
    circular->tail = 0;
    circular->wrapPoint = 0;
}

static bool CircularBuffer_Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead)
{
    struct SolidSyslogCircularBuffer* circular = (struct SolidSyslogCircularBuffer*) self;
    SolidSyslogMutex_Lock(circular->mutex);
    *bytesRead = 0;
    bool delivered = !CircularBuffer_IsEmpty(circular);
    if (delivered)
    {
        if (CircularBuffer_HeadAtWrapPoint(circular))
        {
            CircularBuffer_ConsumeWrapMarker(circular);
        }
        if (CircularBuffer_PeekRecordSize(circular) <= maxSize)
        {
            CircularBuffer_LoadRecord(circular, data, bytesRead);
        }
        else
        {
            delivered = false;
        }
    }
    SolidSyslogMutex_Unlock(circular->mutex);
    return delivered;
}

static inline bool CircularBuffer_IsEmpty(const struct SolidSyslogCircularBuffer* circular)
{
    return circular->head == circular->tail;
}

static inline bool CircularBuffer_HeadAtWrapPoint(const struct SolidSyslogCircularBuffer* circular)
{
    return circular->head >= circular->wrapPoint;
}

static inline void CircularBuffer_ConsumeWrapMarker(struct SolidSyslogCircularBuffer* circular)
{
    circular->head = 0;
    circular->wrapPoint = circular->capacity;
}

static inline size_t CircularBuffer_PeekRecordSize(const struct SolidSyslogCircularBuffer* circular)
{
    uint16_t header = 0;
    memcpy(&header, &circular->storage[circular->head], HEADER_BYTES);
    return header;
}

static inline void CircularBuffer_LoadRecord(struct SolidSyslogCircularBuffer* circular, void* data, size_t* bytesRead)
{
    size_t recordSize = CircularBuffer_PeekRecordSize(circular);
    memcpy(data, &circular->storage[circular->head + HEADER_BYTES], recordSize);
    circular->head += HEADER_BYTES + recordSize;
    *bytesRead = recordSize;
}

static void CircularBuffer_Write(struct SolidSyslogBuffer* self, const void* data, size_t size)
{
    struct SolidSyslogCircularBuffer* circular = (struct SolidSyslogCircularBuffer*) self;
    if (size <= SOLIDSYSLOG_MAX_MESSAGE_SIZE)
    {
        SolidSyslogMutex_Lock(circular->mutex);
        if (CircularBuffer_IsEmpty(circular))
        {
            CircularBuffer_ResetToStart(circular);
        }
        size_t recordBytes = HEADER_BYTES + size;
        bool fitsAtTail = CircularBuffer_RecordFitsAtTail(circular, recordBytes);
        bool shouldWrite = fitsAtTail || CircularBuffer_RecordFitsAfterWrap(circular, recordBytes);
        if (shouldWrite)
        {
            if (!fitsAtTail)
            {
                CircularBuffer_WrapTail(circular);
            }
            CircularBuffer_StoreRecord(circular, data, size);
        }
        SolidSyslogMutex_Unlock(circular->mutex);
    }
}

static inline bool CircularBuffer_IsWrapped(const struct SolidSyslogCircularBuffer* circular)
{
    return circular->head > circular->tail;
}

static inline bool CircularBuffer_RecordFitsAtTail(const struct SolidSyslogCircularBuffer* circular, size_t recordBytes)
{
    if (CircularBuffer_IsWrapped(circular))
    {
        return circular->tail + recordBytes < circular->head;
    }
    return circular->tail + recordBytes <= circular->capacity;
}

static inline bool CircularBuffer_RecordFitsAfterWrap(
    const struct SolidSyslogCircularBuffer* circular,
    size_t recordBytes
)
{
    return (!CircularBuffer_IsWrapped(circular)) && recordBytes < circular->head;
}

static inline void CircularBuffer_ResetToStart(struct SolidSyslogCircularBuffer* circular)
{
    circular->head = 0;
    circular->tail = 0;
    circular->wrapPoint = circular->capacity;
}

static inline void CircularBuffer_WrapTail(struct SolidSyslogCircularBuffer* circular)
{
    circular->wrapPoint = circular->tail;
    circular->tail = 0;
}

static inline void CircularBuffer_StoreRecord(struct SolidSyslogCircularBuffer* circular, const void* data, size_t size)
{
    uint16_t header = (uint16_t) size;
    memcpy(&circular->storage[circular->tail], &header, HEADER_BYTES);
    memcpy(&circular->storage[circular->tail + HEADER_BYTES], data, size);
    circular->tail += HEADER_BYTES + size;
}
