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
    size_t                   capacity;
    size_t                   head;
    size_t                   tail;
    size_t                   wrapPoint;
    uint8_t                  storage[];
};

SOLIDSYSLOG_STATIC_ASSERT(sizeof(struct SolidSyslogCircularBuffer) == SOLIDSYSLOG_CIRCULARBUFFER_OVERHEAD * sizeof(SolidSyslogCircularBufferStorage),
                          "SOLIDSYSLOG_CIRCULARBUFFER_OVERHEAD does not match struct layout");

static bool Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead);
static void Write(struct SolidSyslogBuffer* self, const void* data, size_t size);

static inline bool   IsEmpty(const struct SolidSyslogCircularBuffer* circular);
static inline bool   IsWrapped(const struct SolidSyslogCircularBuffer* circular);
static inline bool   HeadAtWrapPoint(const struct SolidSyslogCircularBuffer* circular);
static inline bool   RecordFitsAtTail(const struct SolidSyslogCircularBuffer* circular, size_t recordBytes);
static inline bool   RecordFitsAfterWrap(const struct SolidSyslogCircularBuffer* circular, size_t recordBytes);
static inline void   ResetToStart(struct SolidSyslogCircularBuffer* circular);
static inline void   WrapTail(struct SolidSyslogCircularBuffer* circular);
static inline void   ConsumeWrapMarker(struct SolidSyslogCircularBuffer* circular);
static inline void   StoreRecord(struct SolidSyslogCircularBuffer* circular, const void* data, size_t size);
static inline void   LoadRecord(struct SolidSyslogCircularBuffer* circular, void* data, size_t* bytesRead);
static inline size_t PeekRecordSize(const struct SolidSyslogCircularBuffer* circular);

struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(SolidSyslogCircularBufferStorage* storage, size_t storageBytes, struct SolidSyslogMutex* mutex)
{
    struct SolidSyslogCircularBuffer* circular = (struct SolidSyslogCircularBuffer*) storage;
    circular->base.Read                        = Read;
    circular->base.Write                       = Write;
    circular->mutex                            = mutex;
    circular->capacity                         = storageBytes - sizeof(struct SolidSyslogCircularBuffer);
    ResetToStart(circular);
    return &circular->base;
}

void SolidSyslogCircularBuffer_Destroy(struct SolidSyslogBuffer* buffer)
{
    struct SolidSyslogCircularBuffer* circular = (struct SolidSyslogCircularBuffer*) buffer;
    circular->base.Read                        = NULL;
    circular->base.Write                       = NULL;
    circular->mutex                            = NULL;
    circular->capacity                         = 0;
    circular->head                             = 0;
    circular->tail                             = 0;
    circular->wrapPoint                        = 0;
}

static bool Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead)
{
    struct SolidSyslogCircularBuffer* circular = (struct SolidSyslogCircularBuffer*) self;
    SolidSyslogMutex_Lock(circular->mutex);
    *bytesRead     = 0;
    bool delivered = !IsEmpty(circular);
    if (delivered)
    {
        if (HeadAtWrapPoint(circular))
        {
            ConsumeWrapMarker(circular);
        }
        if (PeekRecordSize(circular) <= maxSize)
        {
            LoadRecord(circular, data, bytesRead);
        }
        else
        {
            delivered = false;
        }
    }
    SolidSyslogMutex_Unlock(circular->mutex);
    return delivered;
}

static inline bool IsEmpty(const struct SolidSyslogCircularBuffer* circular)
{
    return circular->head == circular->tail;
}

static inline bool HeadAtWrapPoint(const struct SolidSyslogCircularBuffer* circular)
{
    return circular->head >= circular->wrapPoint;
}

static inline void ConsumeWrapMarker(struct SolidSyslogCircularBuffer* circular)
{
    circular->head      = 0;
    circular->wrapPoint = circular->capacity;
}

static inline size_t PeekRecordSize(const struct SolidSyslogCircularBuffer* circular)
{
    uint16_t header = 0;
    memcpy(&header, &circular->storage[circular->head], HEADER_BYTES);
    return header;
}

static inline void LoadRecord(struct SolidSyslogCircularBuffer* circular, void* data, size_t* bytesRead)
{
    size_t recordSize = PeekRecordSize(circular);
    memcpy(data, &circular->storage[circular->head + HEADER_BYTES], recordSize);
    circular->head += HEADER_BYTES + recordSize;
    *bytesRead = recordSize;
}

static void Write(struct SolidSyslogBuffer* self, const void* data, size_t size)
{
    struct SolidSyslogCircularBuffer* circular = (struct SolidSyslogCircularBuffer*) self;
    if (size <= SOLIDSYSLOG_MAX_MESSAGE_SIZE)
    {
        SolidSyslogMutex_Lock(circular->mutex);
        if (IsEmpty(circular))
        {
            ResetToStart(circular);
        }
        size_t recordBytes = HEADER_BYTES + size;
        bool   fitsAtTail  = RecordFitsAtTail(circular, recordBytes);
        bool   shouldWrite = fitsAtTail || RecordFitsAfterWrap(circular, recordBytes);
        if (shouldWrite)
        {
            if (!fitsAtTail)
            {
                WrapTail(circular);
            }
            StoreRecord(circular, data, size);
        }
        SolidSyslogMutex_Unlock(circular->mutex);
    }
}

static inline bool IsWrapped(const struct SolidSyslogCircularBuffer* circular)
{
    return circular->head > circular->tail;
}

static inline bool RecordFitsAtTail(const struct SolidSyslogCircularBuffer* circular, size_t recordBytes)
{
    if (IsWrapped(circular))
    {
        return circular->tail + recordBytes < circular->head;
    }
    return circular->tail + recordBytes <= circular->capacity;
}

static inline bool RecordFitsAfterWrap(const struct SolidSyslogCircularBuffer* circular, size_t recordBytes)
{
    return (!IsWrapped(circular)) && recordBytes < circular->head;
}

static inline void ResetToStart(struct SolidSyslogCircularBuffer* circular)
{
    circular->head      = 0;
    circular->tail      = 0;
    circular->wrapPoint = circular->capacity;
}

static inline void WrapTail(struct SolidSyslogCircularBuffer* circular)
{
    circular->wrapPoint = circular->tail;
    circular->tail      = 0;
}

static inline void StoreRecord(struct SolidSyslogCircularBuffer* circular, const void* data, size_t size)
{
    uint16_t header = (uint16_t) size;
    memcpy(&circular->storage[circular->tail], &header, HEADER_BYTES);
    memcpy(&circular->storage[circular->tail + HEADER_BYTES], data, size);
    circular->tail += HEADER_BYTES + size;
}
