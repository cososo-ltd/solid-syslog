/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64AtomicCounter.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullAtomicCounter.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogVxWorks64AtomicCounterErrors.h"
#include "SolidSyslogVxWorks64AtomicCounterPrivate.h"

struct SolidSyslogAtomicCounter;

static inline size_t VxWorks64AtomicCounter_IndexFromHandle(const struct SolidSyslogAtomicCounter* base);
static inline void VxWorks64AtomicCounter_CleanupAtIndex(size_t index, void* context);

static bool VxWorks64AtomicCounter_InUse[SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE];
static struct SolidSyslogVxWorks64AtomicCounter VxWorks64AtomicCounter_Pool[SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE];
static struct SolidSyslogPoolAllocator VxWorks64AtomicCounter_Allocator = {
    VxWorks64AtomicCounter_InUse,
    SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE
};

struct SolidSyslogAtomicCounter* SolidSyslogVxWorks64AtomicCounter_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&VxWorks64AtomicCounter_Allocator);
    struct SolidSyslogAtomicCounter* handle = SolidSyslogNullAtomicCounter_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&VxWorks64AtomicCounter_Allocator, index) == true)
    {
        SolidSyslogVxWorks64AtomicCounter_Initialise(&VxWorks64AtomicCounter_Pool[index].Base);
        handle = &VxWorks64AtomicCounter_Pool[index].Base;
    }
    else
    {
        VxWorks64AtomicCounter_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_ATOMIC_COUNTER_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogVxWorks64AtomicCounter_Destroy(struct SolidSyslogAtomicCounter* base)
{
    size_t index = VxWorks64AtomicCounter_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&VxWorks64AtomicCounter_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &VxWorks64AtomicCounter_Allocator,
                        index,
                        VxWorks64AtomicCounter_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        VxWorks64AtomicCounter_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_ATOMIC_COUNTER_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t VxWorks64AtomicCounter_IndexFromHandle(const struct SolidSyslogAtomicCounter* base)
{
    size_t result = SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE; poolIndex++)
    {
        if (base == &VxWorks64AtomicCounter_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void VxWorks64AtomicCounter_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogVxWorks64AtomicCounter_Cleanup(&VxWorks64AtomicCounter_Pool[index].Base);
}
