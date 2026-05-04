#include "SolidSyslogCircularBuffer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "SolidSyslogBufferDefinition.h"

enum
{
    STORAGE_BYTES = 512,
    HEADER_BYTES  = sizeof(uint16_t)
};

struct SolidSyslogCircularBuffer
{
    struct SolidSyslogBuffer base;
    uint8_t                  storage[STORAGE_BYTES];
    size_t                   head;
    size_t                   tail;
    size_t                   wrapPoint;
};

static bool Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead);
static void Write(struct SolidSyslogBuffer* self, const void* data, size_t size);

static inline bool IsEmpty(const struct SolidSyslogCircularBuffer* circular);
static inline bool IsWrapped(const struct SolidSyslogCircularBuffer* circular);
static inline bool HeadAtWrapPoint(const struct SolidSyslogCircularBuffer* circular);
static inline bool RecordFitsAtTail(const struct SolidSyslogCircularBuffer* circular, size_t recordBytes);
static inline bool RecordFitsAfterWrap(const struct SolidSyslogCircularBuffer* circular, size_t recordBytes);
static inline void ResetToStart(struct SolidSyslogCircularBuffer* circular);
static inline void WrapTail(struct SolidSyslogCircularBuffer* circular);
static inline void ConsumeWrapMarker(struct SolidSyslogCircularBuffer* circular);
static inline void StoreRecord(struct SolidSyslogCircularBuffer* circular, const void* data, size_t size);
static inline void LoadRecord(struct SolidSyslogCircularBuffer* circular, void* data, size_t* bytesRead);

static struct SolidSyslogCircularBuffer instance;

struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(void)
{
    instance.base.Read  = Read;
    instance.base.Write = Write;
    ResetToStart(&instance);
    return &instance.base;
}

void SolidSyslogCircularBuffer_Destroy(void)
{
    instance.base.Read  = NULL;
    instance.base.Write = NULL;
    ResetToStart(&instance);
}

static bool Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead)
{
    struct SolidSyslogCircularBuffer* circular = (struct SolidSyslogCircularBuffer*) self;
    (void) maxSize;
    *bytesRead     = 0;
    bool hadRecord = !IsEmpty(circular);
    if (hadRecord)
    {
        if (HeadAtWrapPoint(circular))
        {
            ConsumeWrapMarker(circular);
        }
        LoadRecord(circular, data, bytesRead);
    }
    return hadRecord;
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
    circular->wrapPoint = STORAGE_BYTES;
}

static inline void LoadRecord(struct SolidSyslogCircularBuffer* circular, void* data, size_t* bytesRead)
{
    uint16_t header;
    memcpy(&header, &circular->storage[circular->head], HEADER_BYTES);
    size_t recordSize = header;
    memcpy(data, &circular->storage[circular->head + HEADER_BYTES], recordSize);
    circular->head += HEADER_BYTES + recordSize;
    *bytesRead = recordSize;
}

static void Write(struct SolidSyslogBuffer* self, const void* data, size_t size)
{
    struct SolidSyslogCircularBuffer* circular = (struct SolidSyslogCircularBuffer*) self;
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
}

static inline bool IsWrapped(const struct SolidSyslogCircularBuffer* circular)
{
    return circular->head > circular->tail;
}

static inline bool RecordFitsAtTail(const struct SolidSyslogCircularBuffer* circular, size_t recordBytes)
{
    size_t limit = IsWrapped(circular) ? circular->head : (size_t) STORAGE_BYTES;
    return circular->tail + recordBytes <= limit;
}

static inline bool RecordFitsAfterWrap(const struct SolidSyslogCircularBuffer* circular, size_t recordBytes)
{
    return (!IsWrapped(circular)) && recordBytes <= circular->head;
}

static inline void ResetToStart(struct SolidSyslogCircularBuffer* circular)
{
    circular->head      = 0;
    circular->tail      = 0;
    circular->wrapPoint = STORAGE_BYTES;
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
