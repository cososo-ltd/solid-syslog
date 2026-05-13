#include "SolidSyslogNullStore.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogStoreDefinition.h"

static bool   Write(struct SolidSyslogStore* self, const void* data, size_t size);
static bool   ReadNextUnsent(struct SolidSyslogStore* self, void* data, size_t maxSize, size_t* bytesRead);
static void   MarkSent(struct SolidSyslogStore* self);
static bool   HasUnsent(struct SolidSyslogStore* self);
static bool   IsHalted(struct SolidSyslogStore* self);
static size_t GetTotalBytes(struct SolidSyslogStore* self);
static size_t GetUsedBytes(struct SolidSyslogStore* self);
static bool   IsTransient(struct SolidSyslogStore* self);

struct SolidSyslogNullStore
{
    struct SolidSyslogStore base;
};

static struct SolidSyslogNullStore instance;

struct SolidSyslogStore* SolidSyslogNullStore_Create(void)
{
    instance.base.Write          = Write;
    instance.base.ReadNextUnsent = ReadNextUnsent;
    instance.base.MarkSent       = MarkSent;
    instance.base.HasUnsent      = HasUnsent;
    instance.base.IsHalted       = IsHalted;
    instance.base.GetTotalBytes  = GetTotalBytes;
    instance.base.GetUsedBytes   = GetUsedBytes;
    instance.base.IsTransient    = IsTransient;
    return &instance.base;
}

void SolidSyslogNullStore_Destroy(void)
{
    instance.base.Write          = NULL;
    instance.base.ReadNextUnsent = NULL;
    instance.base.MarkSent       = NULL;
    instance.base.HasUnsent      = NULL;
    instance.base.IsHalted       = NULL;
    instance.base.GetTotalBytes  = NULL;
    instance.base.GetUsedBytes   = NULL;
    instance.base.IsTransient    = NULL;
}

/* NullStore never retains. Returns false to signal "not held by this store"
 * so the eager-drain loop in ProcessMessages takes the direct-send path —
 * NullStore + real-buffer + UDP is the constrained-system "one attempt per
 * message, no buffering" configuration. */
static bool Write(struct SolidSyslogStore* self, const void* data, size_t size)
{
    (void) self;
    (void) data;
    (void) size;
    return false;
}

static bool ReadNextUnsent(struct SolidSyslogStore* self, void* data, size_t maxSize, size_t* bytesRead)
{
    (void) self;
    (void) data;
    (void) maxSize;
    *bytesRead = 0;
    return false;
}

static void MarkSent(struct SolidSyslogStore* self)
{
    (void) self;
}

static bool HasUnsent(struct SolidSyslogStore* self)
{
    (void) self;
    return false;
}

static bool IsHalted(struct SolidSyslogStore* self)
{
    (void) self;
    return false;
}

static size_t GetTotalBytes(struct SolidSyslogStore* self)
{
    (void) self;
    return 0;
}

static size_t GetUsedBytes(struct SolidSyslogStore* self)
{
    (void) self;
    return 0;
}

/* NullStore retains nothing — a Write rejection means "I never had it,
 * please try the sender." Service's DrainBufferIntoStore consults this
 * to know it's safe to fall through to direct-send. */
static bool IsTransient(struct SolidSyslogStore* self)
{
    (void) self;
    return true;
}
