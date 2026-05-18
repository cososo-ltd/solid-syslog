#include "SolidSyslogPoolAllocator.h"

#include <stddef.h>

#include "SolidSyslogConfigLock.h"

static bool PoolAllocator_TryClaim(struct SolidSyslogPoolAllocator* self, size_t index);
static inline bool PoolAllocator_SlotIsFree(const struct SolidSyslogPoolAllocator* self, size_t index);
static inline bool PoolAllocator_SlotIsInUse(const struct SolidSyslogPoolAllocator* self, size_t index);
static inline void PoolAllocator_MarkInUse(struct SolidSyslogPoolAllocator* self, size_t index);
static inline void PoolAllocator_MarkFree(struct SolidSyslogPoolAllocator* self, size_t index);

size_t SolidSyslogPoolAllocator_AcquireFirstFree(struct SolidSyslogPoolAllocator* self)
{
    size_t acquired = 0;
    while ((acquired < self->Count) && !PoolAllocator_TryClaim(self, acquired))
    {
        acquired++;
    }
    return acquired;
}

static bool PoolAllocator_TryClaim(struct SolidSyslogPoolAllocator* self, size_t index)
{
    bool claimed = false;
    SolidSyslog_LockConfig();
    if (PoolAllocator_SlotIsFree(self, index))
    {
        PoolAllocator_MarkInUse(self, index);
        claimed = true;
    }
    SolidSyslog_UnlockConfig();
    return claimed;
}

static inline bool PoolAllocator_SlotIsFree(const struct SolidSyslogPoolAllocator* self, size_t index)
{
    return !PoolAllocator_SlotIsInUse(self, index);
}

static inline bool PoolAllocator_SlotIsInUse(const struct SolidSyslogPoolAllocator* self, size_t index)
{
    return self->InUse[index];
}

static inline void PoolAllocator_MarkInUse(struct SolidSyslogPoolAllocator* self, size_t index)
{
    self->InUse[index] = true;
}

bool SolidSyslogPoolAllocator_FreeIfInUse(
    struct SolidSyslogPoolAllocator* self,
    size_t index,
    SolidSyslogPoolCleanup cleanup,
    void* context
)
{
    SolidSyslog_LockConfig();
    bool released = PoolAllocator_SlotIsInUse(self, index);
    if (released)
    {
        if (cleanup != NULL)
        {
            cleanup(index, context);
        }
        PoolAllocator_MarkFree(self, index);
    }
    SolidSyslog_UnlockConfig();
    return released;
}

static inline void PoolAllocator_MarkFree(struct SolidSyslogPoolAllocator* self, size_t index)
{
    self->InUse[index] = false;
}
