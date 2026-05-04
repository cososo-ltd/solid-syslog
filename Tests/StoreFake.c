#include "StoreFake.h"

#include <string.h>
#include <stdbool.h>

#include "SolidSyslogStoreDefinition.h"
#include "TestUtils.h"

enum
{
    STOREFAKE_MAX_SIZE = 1024
};

static bool Write(struct SolidSyslogStore* self, const void* data, size_t size);
static bool ReadNextUnsent(struct SolidSyslogStore* self, void* data, size_t maxSize, size_t* bytesRead);
static void MarkSent(struct SolidSyslogStore* self);
static bool HasUnsent(struct SolidSyslogStore* self);
static bool IsHalted(struct SolidSyslogStore* self);

struct StoreFake
{
    struct SolidSyslogStore base;
    char                    stored[STOREFAKE_MAX_SIZE];
    size_t                  storedSize;
    bool                    unsent;
    bool                    failNextWrite;
    bool                    failNextRead;
    bool                    halted;
};

static struct StoreFake instance;

struct SolidSyslogStore* StoreFake_Create(void)
{
    instance                     = (struct StoreFake) {0};
    instance.base.Write          = Write;
    instance.base.ReadNextUnsent = ReadNextUnsent;
    instance.base.MarkSent       = MarkSent;
    instance.base.HasUnsent      = HasUnsent;
    instance.base.IsHalted       = IsHalted;
    return &instance.base;
}

void StoreFake_Destroy(void)
{
    instance = (struct StoreFake) {0};
}

void StoreFake_FailNextWrite(void)
{
    instance.failNextWrite = true;
}

void StoreFake_FailNextRead(void)
{
    instance.failNextRead = true;
}

static bool Write(struct SolidSyslogStore* self, const void* data, size_t size)
{
    struct StoreFake* fake = (struct StoreFake*) self;

    if (fake->failNextWrite)
    {
        fake->failNextWrite = false;
        return false;
    }

    if (!fake->unsent)
    {
        size_t copySize = MinSize(size, sizeof(fake->stored));
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) -- memcpy with bounded copySize; memcpy_s is not portable
        memcpy(fake->stored, data, copySize);
        fake->storedSize = copySize;
        fake->unsent     = true;
    }
    return true;
}

static bool ReadNextUnsent(struct SolidSyslogStore* self, void* data, size_t maxSize, size_t* bytesRead)
{
    struct StoreFake* fake = (struct StoreFake*) self;

    if (fake->failNextRead)
    {
        fake->failNextRead = false;
        *bytesRead         = 0;
        return false;
    }

    bool success = fake->unsent;

    if (success)
    {
        size_t copySize = MinSize(fake->storedSize, maxSize);
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) -- memcpy with bounded copySize; memcpy_s is not portable
        memcpy(data, fake->stored, copySize);
        *bytesRead = copySize;
    }

    return success;
}

static void MarkSent(struct SolidSyslogStore* self)
{
    struct StoreFake* fake = (struct StoreFake*) self;
    fake->unsent           = false;
}

static bool HasUnsent(struct SolidSyslogStore* self)
{
    struct StoreFake* fake = (struct StoreFake*) self;
    return fake->unsent;
}

static bool IsHalted(struct SolidSyslogStore* self)
{
    struct StoreFake* fake = (struct StoreFake*) self;
    return fake->halted;
}

void StoreFake_SetHalted(void)
{
    instance.halted = true;
}
