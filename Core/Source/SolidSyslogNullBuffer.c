#include "SolidSyslogNullBuffer.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogSender.h"

static bool NullBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
static void NullBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size);

static inline struct SolidSyslogNullBuffer* NullBuffer_SelfFromBase(struct SolidSyslogBuffer* base);

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

static bool NullBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead)
{
    (void) base;
    (void) data;
    (void) maxSize;
    *bytesRead = 0;
    return false;
}

static void NullBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size)
{
    struct SolidSyslogNullBuffer* self = NullBuffer_SelfFromBase(base);
    SolidSyslogSender_Send(self->Sender, data, size);
}

static inline struct SolidSyslogNullBuffer* NullBuffer_SelfFromBase(struct SolidSyslogBuffer* base)
{
    return (struct SolidSyslogNullBuffer*) base;
}
