/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGPOOLALLOCATOR_H
#define SOLIDSYSLOGPOOLALLOCATOR_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>
#include <stddef.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

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

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOOLALLOCATOR_H */
