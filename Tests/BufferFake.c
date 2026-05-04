#include "BufferFake.h"

#include <string.h>
#include <stdbool.h>

#include "SolidSyslogBufferDefinition.h"
#include "TestUtils.h"

enum
{
    BUFFERFAKE_MAX_SIZE = 1024
};

static bool Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead);
static void Write(struct SolidSyslogBuffer* self, const void* data, size_t size);

struct BufferFake
{
    struct SolidSyslogBuffer base;
    char                     stored[BUFFERFAKE_MAX_SIZE];
    size_t                   storedSize;
    bool                     pending;
};

static struct BufferFake instance;

struct SolidSyslogBuffer* BufferFake_Create(void)
{
    instance            = (struct BufferFake) {0};
    instance.base.Write = Write;
    instance.base.Read  = Read;
    return &instance.base;
}

void BufferFake_Destroy(void)
{
    instance = (struct BufferFake) {0};
}

static bool Read(struct SolidSyslogBuffer* self, void* data, size_t maxSize, size_t* bytesRead)
{
    struct BufferFake* fake    = (struct BufferFake*) self;
    bool               success = fake->pending;

    if (success)
    {
        size_t copySize = MinSize(fake->storedSize, maxSize);
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) -- memcpy with bounded copySize; memcpy_s is not portable
        memcpy(data, fake->stored, copySize);
        *bytesRead    = copySize;
        fake->pending = false;
    }

    return success;
}

static void Write(struct SolidSyslogBuffer* self, const void* data, size_t size)
{
    struct BufferFake* fake = (struct BufferFake*) self;

    size_t copySize = MinSize(size, sizeof(fake->stored));
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) -- memcpy with bounded copySize; memcpy_s is not portable
    memcpy(fake->stored, data, copySize);
    fake->storedSize = copySize;
    fake->pending    = true;
}
