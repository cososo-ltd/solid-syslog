#include "SolidSyslogCircularBuffer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "SolidSyslogBufferDefinition.h"

enum
{
    STORAGE_BYTES = 512
};

static bool Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead);
static void Write(struct SolidSyslogBuffer* self, const void* data, size_t size);

struct SolidSyslogCircularBuffer
{
    struct SolidSyslogBuffer base;
    uint8_t                  storage[STORAGE_BYTES];
    size_t                   head;
    size_t                   tail;
};

static struct SolidSyslogCircularBuffer instance;

struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(void)
{
    instance.base.Read  = Read;
    instance.base.Write = Write;
    instance.head       = 0;
    instance.tail       = 0;
    return &instance.base;
}

void SolidSyslogCircularBuffer_Destroy(void)
{
    instance.base.Read  = NULL;
    instance.base.Write = NULL;
    instance.head       = 0;
    instance.tail       = 0;
}

static bool Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead)
{
    struct SolidSyslogCircularBuffer* circular = (struct SolidSyslogCircularBuffer*) self;
    (void) maxSize;
    bool hadRecord = circular->head != circular->tail;
    *bytesRead     = 0;
    if (hadRecord)
    {
        uint16_t header;
        memcpy(&header, &circular->storage[circular->head], sizeof(header));
        size_t recordSize = header;
        memcpy(data, &circular->storage[circular->head + sizeof(header)], recordSize);
        circular->head += sizeof(header) + recordSize;
        *bytesRead = recordSize;
    }
    return hadRecord;
}

static void Write(struct SolidSyslogBuffer* self, const void* data, size_t size)
{
    struct SolidSyslogCircularBuffer* circular = (struct SolidSyslogCircularBuffer*) self;
    uint16_t                          header   = (uint16_t) size;
    memcpy(&circular->storage[circular->tail], &header, sizeof(header));
    memcpy(&circular->storage[circular->tail + sizeof(header)], data, size);
    circular->tail += sizeof(header) + size;
}
