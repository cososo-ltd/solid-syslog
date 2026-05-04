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
    bool                     hasRecord;
    size_t                   recordSize;
};

static struct SolidSyslogCircularBuffer instance;

struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(void)
{
    instance.base.Read  = Read;
    instance.base.Write = Write;
    instance.hasRecord  = false;
    instance.recordSize = 0;
    return &instance.base;
}

void SolidSyslogCircularBuffer_Destroy(void)
{
    instance.base.Read  = NULL;
    instance.base.Write = NULL;
    instance.hasRecord  = false;
}

static bool Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead)
{
    struct SolidSyslogCircularBuffer* circular = (struct SolidSyslogCircularBuffer*) self;
    (void) maxSize;
    *bytesRead          = circular->recordSize;
    bool hadRecord      = circular->hasRecord;
    circular->hasRecord = false;
    if (hadRecord)
    {
        memcpy(data, circular->storage, circular->recordSize);
    }
    return hadRecord;
}

static void Write(struct SolidSyslogBuffer* self, const void* data, size_t size)
{
    struct SolidSyslogCircularBuffer* circular = (struct SolidSyslogCircularBuffer*) self;
    memcpy(circular->storage, data, size);
    circular->hasRecord  = true;
    circular->recordSize = size;
}
