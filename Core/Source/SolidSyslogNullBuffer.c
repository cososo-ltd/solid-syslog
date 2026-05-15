#include "SolidSyslogNullBuffer.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogSender.h"

static bool NullBuffer_Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead);
static void NullBuffer_Write(struct SolidSyslogBuffer* self, const void* data, size_t size);

struct SolidSyslogNullBuffer
{
    struct SolidSyslogBuffer Base;
    struct SolidSyslogSender* Sender;
};

static struct SolidSyslogNullBuffer instance;

struct SolidSyslogBuffer* SolidSyslogNullBuffer_Create(struct SolidSyslogSender* sender)
{
    instance.Base.Write = NullBuffer_Write;
    instance.Base.Read = NullBuffer_Read;
    instance.Sender = sender;
    return &instance.Base;
}

void SolidSyslogNullBuffer_Destroy(void)
{
    instance.Base.Write = NULL;
    instance.Base.Read = NULL;
    instance.Sender = NULL;
}

static bool NullBuffer_Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead)
{
    (void) self;
    (void) data;
    (void) maxSize;
    *bytesRead = 0;
    return false;
}

static void NullBuffer_Write(struct SolidSyslogBuffer* self, const void* data, size_t size)
{
    struct SolidSyslogNullBuffer* nullBuffer = (struct SolidSyslogNullBuffer*) self;
    SolidSyslogSender_Send(nullBuffer->Sender, data, size);
}
