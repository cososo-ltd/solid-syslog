#include "SolidSyslogNullStore.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogStoreDefinition.h"

static bool NullStore_Write(struct SolidSyslogStore* base, const void* data, size_t size);
static bool NullStore_ReadNextUnsent(struct SolidSyslogStore* base, void* data, size_t maxSize, size_t* bytesRead);
static void NullStore_MarkSent(struct SolidSyslogStore* base);
static bool NullStore_HasUnsent(struct SolidSyslogStore* base);
static bool NullStore_IsHalted(struct SolidSyslogStore* base);
static size_t NullStore_GetTotalBytes(struct SolidSyslogStore* base);
static size_t NullStore_GetUsedBytes(struct SolidSyslogStore* base);
static bool NullStore_IsTransient(struct SolidSyslogStore* base);

struct SolidSyslogNullStore
{
    struct SolidSyslogStore Base;
};

static struct SolidSyslogNullStore instance;

struct SolidSyslogStore* SolidSyslogNullStore_Create(void)
{
    instance.Base.Write = NullStore_Write;
    instance.Base.ReadNextUnsent = NullStore_ReadNextUnsent;
    instance.Base.MarkSent = NullStore_MarkSent;
    instance.Base.HasUnsent = NullStore_HasUnsent;
    instance.Base.IsHalted = NullStore_IsHalted;
    instance.Base.GetTotalBytes = NullStore_GetTotalBytes;
    instance.Base.GetUsedBytes = NullStore_GetUsedBytes;
    instance.Base.IsTransient = NullStore_IsTransient;
    return &instance.Base;
}

void SolidSyslogNullStore_Destroy(void)
{
    instance.Base.Write = NULL;
    instance.Base.ReadNextUnsent = NULL;
    instance.Base.MarkSent = NULL;
    instance.Base.HasUnsent = NULL;
    instance.Base.IsHalted = NULL;
    instance.Base.GetTotalBytes = NULL;
    instance.Base.GetUsedBytes = NULL;
    instance.Base.IsTransient = NULL;
}

/* NullStore never retains. Returns false to signal "not held by this store"
 * so the eager-drain loop in ProcessMessages takes the direct-send path —
 * NullStore + real-buffer + UDP is the constrained-system "one attempt per
 * message, no buffering" configuration. */
static bool NullStore_Write(struct SolidSyslogStore* base, const void* data, size_t size)
{
    (void) base;
    (void) data;
    (void) size;
    return false;
}

static bool NullStore_ReadNextUnsent(struct SolidSyslogStore* base, void* data, size_t maxSize, size_t* bytesRead)
{
    (void) base;
    (void) data;
    (void) maxSize;
    *bytesRead = 0;
    return false;
}

static void NullStore_MarkSent(struct SolidSyslogStore* base)
{
    (void) base;
}

static bool NullStore_HasUnsent(struct SolidSyslogStore* base)
{
    (void) base;
    return false;
}

static bool NullStore_IsHalted(struct SolidSyslogStore* base)
{
    (void) base;
    return false;
}

static size_t NullStore_GetTotalBytes(struct SolidSyslogStore* base)
{
    (void) base;
    return 0;
}

static size_t NullStore_GetUsedBytes(struct SolidSyslogStore* base)
{
    (void) base;
    return 0;
}

/* NullStore retains nothing — a NullStore_Write rejection means "I never had it,
 * please try the sender." Service's DrainBufferIntoStore consults this
 * to know it's safe to fall through to direct-send. */
static bool NullStore_IsTransient(struct SolidSyslogStore* base)
{
    (void) base;
    return true;
}
