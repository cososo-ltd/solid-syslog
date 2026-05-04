#include "SolidSyslogNullBuffer.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogSender.h"

static bool Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead);
static void Write(struct SolidSyslogBuffer* self, const void* data, size_t size);

struct SolidSyslogNullBuffer
{
    struct SolidSyslogBuffer  base;
    struct SolidSyslogSender* sender;
};

static struct SolidSyslogNullBuffer instance;

struct SolidSyslogBuffer* SolidSyslogNullBuffer_Create(struct SolidSyslogSender* sender)
{
    instance.base.Write = Write;
    instance.base.Read  = Read;
    instance.sender     = sender;
    return &instance.base;
}

void SolidSyslogNullBuffer_Destroy(void)
{
    instance.base.Write = NULL;
    instance.base.Read  = NULL;
    instance.sender     = NULL;
}

static bool Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead)
{
    (void) self;
    (void) data;
    (void) maxSize;
    *bytesRead = 0;
    return false;
}

static void Write(struct SolidSyslogBuffer* self, const void* data, size_t size)
{
    struct SolidSyslogNullBuffer* nullBuffer = (struct SolidSyslogNullBuffer*) self;
    SolidSyslogSender_Send(nullBuffer->sender, data, size);
}
