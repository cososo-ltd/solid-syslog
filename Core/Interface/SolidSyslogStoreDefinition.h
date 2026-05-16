#ifndef SOLIDSYSLOGSTOREDEFINITION_H
#define SOLIDSYSLOGSTOREDEFINITION_H

#include <stdbool.h>
#include <stddef.h>

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogStore
    {
        bool (*Write)(struct SolidSyslogStore* base, const void* data, size_t size);
        bool (*ReadNextUnsent)(struct SolidSyslogStore* base, void* data, size_t maxSize, size_t* bytesRead);
        void (*MarkSent)(struct SolidSyslogStore* base);
        bool (*HasUnsent)(struct SolidSyslogStore* base);
        bool (*IsHalted)(struct SolidSyslogStore* base);
        size_t (*GetTotalBytes)(struct SolidSyslogStore* base);
        size_t (*GetUsedBytes)(struct SolidSyslogStore* base);
        /* True when the store never retains anything (e.g. NullStore), so
         * a Write rejection means "I never had it, please try the sender."
         * False for stores that actually retain records — a rejection there
         * is the discard policy speaking, and the message must NOT bypass
         * older records via a direct-send fallback. Service consults this
         * after a failed Write to decide whether to fall through. */
        bool (*IsTransient)(struct SolidSyslogStore* base);
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGSTOREDEFINITION_H */
