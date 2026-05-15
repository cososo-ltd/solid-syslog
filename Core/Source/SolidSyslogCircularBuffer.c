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
    circular->Base.Read = CircularBuffer_Read;
    circular->Base.Write = CircularBuffer_Write;
    circular->Mutex = mutex;
    circular->Capacity = storageBytes - sizeof(struct SolidSyslogCircularBuffer);
    CircularBuffer_ResetToStart(circular);
    return &circular->Base;
}

void SolidSyslogCircularBuffer_Destroy(struct SolidSyslogBuffer* buffer)
{
    struct SolidSyslogCircularBuffer* circular = (struct SolidSyslogCircularBuffer*) buffer;
    circular->Base.Read = NULL;
    circular->Base.Write = NULL;
    circular->Mutex = NULL;
    circular->Capacity = 0;
    circular->Head = 0;
    circular->Tail = 0;
    circular->WrapPoint = 0;
}

static bool CircularBuffer_Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead)
{
    struct SolidSyslogCircularBuffer* circular = (struct SolidSyslogCircularBuffer*) self;
    SolidSyslogMutex_Lock(circular->Mutex);
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
    SolidSyslogMutex_Unlock(circular->Mutex);
    return delivered;
}

static inline bool CircularBuffer_IsEmpty(const struct SolidSyslogCircularBuffer* circular)
{
    return circular->Head == circular->Tail;
}

static inline bool CircularBuffer_HeadAtWrapPoint(const struct SolidSyslogCircularBuffer* circular)
{
    return circular->Head >= circular->WrapPoint;
}

static inline void CircularBuffer_ConsumeWrapMarker(struct SolidSyslogCircularBuffer* circular)
{
    circular->Head = 0;
    circular->WrapPoint = circular->Capacity;
}

static inline size_t CircularBuffer_PeekRecordSize(const struct SolidSyslogCircularBuffer* circular)
{
    uint16_t header = 0;
    memcpy(&header, &circular->Storage[circular->Head], HEADER_BYTES);
    return header;
}

static inline void CircularBuffer_LoadRecord(struct SolidSyslogCircularBuffer* circular, void* data, size_t* bytesRead)
{
    size_t recordSize = CircularBuffer_PeekRecordSize(circular);
    memcpy(data, &circular->Storage[circular->Head + HEADER_BYTES], recordSize);
    circular->Head += HEADER_BYTES + recordSize;
    *bytesRead = recordSize;
}

static void CircularBuffer_Write(struct SolidSyslogBuffer* self, const void* data, size_t size)
{
    struct SolidSyslogCircularBuffer* circular = (struct SolidSyslogCircularBuffer*) self;
    if (size <= SOLIDSYSLOG_MAX_MESSAGE_SIZE)
    {
        SolidSyslogMutex_Lock(circular->Mutex);
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
        SolidSyslogMutex_Unlock(circular->Mutex);
    }
}

static inline bool CircularBuffer_IsWrapped(const struct SolidSyslogCircularBuffer* circular)
{
    return circular->Head > circular->Tail;
}

static inline bool CircularBuffer_RecordFitsAtTail(const struct SolidSyslogCircularBuffer* circular, size_t recordBytes)
{
    if (CircularBuffer_IsWrapped(circular))
    {
        return circular->Tail + recordBytes < circular->Head;
    }
    return circular->Tail + recordBytes <= circular->Capacity;
}

static inline bool CircularBuffer_RecordFitsAfterWrap(
    const struct SolidSyslogCircularBuffer* circular,
    size_t recordBytes
)
{
    return (!CircularBuffer_IsWrapped(circular)) && recordBytes < circular->Head;
}

static inline void CircularBuffer_ResetToStart(struct SolidSyslogCircularBuffer* circular)
{
    circular->Head = 0;
    circular->Tail = 0;
    circular->WrapPoint = circular->Capacity;
}

static inline void CircularBuffer_WrapTail(struct SolidSyslogCircularBuffer* circular)
{
    circular->WrapPoint = circular->Tail;
    circular->Tail = 0;
}

static inline void CircularBuffer_StoreRecord(struct SolidSyslogCircularBuffer* circular, const void* data, size_t size)
{
    uint16_t header = (uint16_t) size;
    memcpy(&circular->Storage[circular->Tail], &header, HEADER_BYTES);
    memcpy(&circular->Storage[circular->Tail + HEADER_BYTES], data, size);
    circular->Tail += HEADER_BYTES + size;
}
