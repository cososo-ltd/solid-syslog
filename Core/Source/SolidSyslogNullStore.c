#include "SolidSyslogNullStore.h"
#include "SolidSyslogStoreDefinition.h"

static bool   Write(struct SolidSyslogStore* self, const void* data, size_t size);
static bool   ReadNextUnsent(struct SolidSyslogStore* self, void* data, size_t maxSize, size_t* bytesRead);
static void   MarkSent(struct SolidSyslogStore* self);
static bool   HasUnsent(struct SolidSyslogStore* self);
static bool   IsHalted(struct SolidSyslogStore* self);
static size_t GetTotalBytes(struct SolidSyslogStore* self);
static size_t GetUsedBytes(struct SolidSyslogStore* self);

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
}

static bool Write(struct SolidSyslogStore* self, const void* data, size_t size)
{
    (void) self;
    (void) data;
    (void) size;
    return true;
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
