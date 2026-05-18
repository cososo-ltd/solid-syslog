#include "SolidSyslogPassthroughBuffer.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogPassthroughBufferPrivate.h"
#include "SolidSyslogSender.h"

static bool PassthroughBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
static void PassthroughBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size);

static inline struct SolidSyslogPassthroughBuffer* PassthroughBuffer_SelfFromBase(struct SolidSyslogBuffer* base);

void PassthroughBuffer_Initialise(struct SolidSyslogBuffer* base, struct SolidSyslogSender* sender)
{
    struct SolidSyslogPassthroughBuffer* self = PassthroughBuffer_SelfFromBase(base);
    self->Base.Write = PassthroughBuffer_Write;
    self->Base.Read = PassthroughBuffer_Read;
    self->Sender = sender;
}

void PassthroughBuffer_Cleanup(struct SolidSyslogBuffer* base)
{
    struct SolidSyslogPassthroughBuffer* self = PassthroughBuffer_SelfFromBase(base);
    self->Base.Write = NULL;
    self->Base.Read = NULL;
    self->Sender = NULL;
}

static bool PassthroughBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead)
{
    (void) base;
    (void) data;
    (void) maxSize;
    *bytesRead = 0;
    return false;
}

static void PassthroughBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size)
{
    struct SolidSyslogPassthroughBuffer* self = PassthroughBuffer_SelfFromBase(base);
    (void) SolidSyslogSender_Send(self->Sender, data, size);
}

static inline struct SolidSyslogPassthroughBuffer* PassthroughBuffer_SelfFromBase(struct SolidSyslogBuffer* base)
{
    return (struct SolidSyslogPassthroughBuffer*) base;
}
