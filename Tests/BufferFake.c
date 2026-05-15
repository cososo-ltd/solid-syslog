#include "BufferFake.h"

#include <string.h>
#include <stdbool.h>

#include "SolidSyslogBufferDefinition.h"
#include "MinSize.h"

enum
{
    BUFFERFAKE_MAX_SIZE = 1024
};

static bool Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead);
static void Write(struct SolidSyslogBuffer* self, const void* data, size_t size);

struct BufferFake
{
    struct SolidSyslogBuffer Base;
    char Stored[BUFFERFAKE_MAX_SIZE];
    size_t StoredSize;
    bool Pending;
};

static struct BufferFake instance;

struct SolidSyslogBuffer* BufferFake_Create(void)
{
    instance = (struct BufferFake) {0};
    instance.Base.Write = Write;
    instance.Base.Read = Read;
    return &instance.Base;
}

void BufferFake_Destroy(void)
{
    instance = (struct BufferFake) {0};
}

static bool Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead)
{
    struct BufferFake* fake = (struct BufferFake*) self;
    bool success = fake->Pending;

    if (success)
    {
        size_t copySize = MinSize(fake->StoredSize, maxSize);
        memcpy(data, fake->Stored, copySize);
        *bytesRead = copySize;
        fake->Pending = false;
    }

    return success;
}

static void Write(struct SolidSyslogBuffer* self, const void* data, size_t size)
{
    struct BufferFake* fake = (struct BufferFake*) self;

    size_t copySize = MinSize(size, sizeof(fake->Stored));
    memcpy(fake->Stored, data, copySize);
    fake->StoredSize = copySize;
    fake->Pending = true;
}
