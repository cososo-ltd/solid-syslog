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

static struct SolidSyslogNullBuffer NullBuffer_Instance;

struct SolidSyslogBuffer* SolidSyslogNullBuffer_Create(struct SolidSyslogSender* sender)
{
    NullBuffer_Instance.Base.Write = NullBuffer_Write;
    NullBuffer_Instance.Base.Read = NullBuffer_Read;
    NullBuffer_Instance.Sender = sender;
    return &NullBuffer_Instance.Base;
}

void SolidSyslogNullBuffer_Destroy(void)
{
    NullBuffer_Instance.Base.Write = NULL;
    NullBuffer_Instance.Base.Read = NULL;
    NullBuffer_Instance.Sender = NULL;
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
    (void) SolidSyslogSender_Send(self->Sender, data, size);
}

static inline struct SolidSyslogNullBuffer* NullBuffer_SelfFromBase(struct SolidSyslogBuffer* base)
{
    return (struct SolidSyslogNullBuffer*) base;
}
