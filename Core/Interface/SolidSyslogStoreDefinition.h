#ifndef SOLIDSYSLOGSTOREDEFINITION_H
#define SOLIDSYSLOGSTOREDEFINITION_H

#include "SolidSyslogStore.h"

EXTERN_C_BEGIN

    struct SolidSyslogStore
    {
        bool (*Write)(struct SolidSyslogStore* self, const void* data, size_t size);
        bool (*ReadNextUnsent)(struct SolidSyslogStore* self, void* data, size_t maxSize, size_t* bytesRead);
        void (*MarkSent)(struct SolidSyslogStore* self);
        bool (*HasUnsent)(struct SolidSyslogStore* self);
        bool (*IsHalted)(struct SolidSyslogStore* self);
        size_t (*GetTotalBytes)(struct SolidSyslogStore* self);
        size_t (*GetUsedBytes)(struct SolidSyslogStore* self);
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGSTOREDEFINITION_H */
