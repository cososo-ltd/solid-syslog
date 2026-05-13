#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogStoreDefinition.h"
#include "SolidSyslogStore.h"

bool SolidSyslogStore_Write(struct SolidSyslogStore* store, const void* data, size_t size)
{
    return store->Write(store, data, size);
}

bool SolidSyslogStore_ReadNextUnsent(struct SolidSyslogStore* store, void* data, size_t maxSize, size_t* bytesRead)
{
    return store->ReadNextUnsent(store, data, maxSize, bytesRead);
}

void SolidSyslogStore_MarkSent(struct SolidSyslogStore* store)
{
    store->MarkSent(store);
}

bool SolidSyslogStore_HasUnsent(struct SolidSyslogStore* store)
{
    return store->HasUnsent(store);
}

bool SolidSyslogStore_IsHalted(struct SolidSyslogStore* store)
{
    return store->IsHalted(store);
}

size_t SolidSyslogStore_GetTotalBytes(struct SolidSyslogStore* store)
{
    return store->GetTotalBytes(store);
}

size_t SolidSyslogStore_GetUsedBytes(struct SolidSyslogStore* store)
{
    return store->GetUsedBytes(store);
}

bool SolidSyslogStore_IsTransient(struct SolidSyslogStore* store)
{
    return store->IsTransient(store);
}
