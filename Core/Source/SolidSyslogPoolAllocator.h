#ifndef SOLIDSYSLOGPOOLALLOCATOR_H
#define SOLIDSYSLOGPOOLALLOCATOR_H

#include "ExternC.h"

#include <stdbool.h>
#include <stddef.h>

EXTERN_C_BEGIN

    struct SolidSyslogPoolAllocator
    {
        bool* InUse;
        size_t Count;
    };

    typedef void (*SolidSyslogPoolCleanup)(size_t index, void* context);

    size_t SolidSyslogPoolAllocator_AcquireFirstFree(struct SolidSyslogPoolAllocator * self);

    bool SolidSyslogPoolAllocator_FreeIfInUse(
        struct SolidSyslogPoolAllocator * self,
        size_t index,
        SolidSyslogPoolCleanup cleanup,
        void* context
    );

    static inline bool SolidSyslogPoolAllocator_IndexIsValid(const struct SolidSyslogPoolAllocator* self, size_t index)
    {
        return index < self->Count;
    }

EXTERN_C_END

#endif /* SOLIDSYSLOGPOOLALLOCATOR_H */
