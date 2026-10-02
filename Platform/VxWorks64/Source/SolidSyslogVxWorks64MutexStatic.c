/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Mutex.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogVxWorks64MutexErrors.h"
#include "SolidSyslogVxWorks64MutexPrivate.h"

struct SolidSyslogMutex;

static inline size_t VxWorks64Mutex_IndexFromHandle(const struct SolidSyslogMutex* base);
static inline void VxWorks64Mutex_CleanupAtIndex(size_t index, void* context);

static bool VxWorks64Mutex_InUse[SOLIDSYSLOG_MUTEX_POOL_SIZE];
static struct SolidSyslogVxWorks64Mutex VxWorks64Mutex_Pool[SOLIDSYSLOG_MUTEX_POOL_SIZE];
static struct SolidSyslogPoolAllocator VxWorks64Mutex_Allocator = {VxWorks64Mutex_InUse, SOLIDSYSLOG_MUTEX_POOL_SIZE};

struct SolidSyslogMutex* SolidSyslogVxWorks64Mutex_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&VxWorks64Mutex_Allocator);
    struct SolidSyslogMutex* handle = SolidSyslogNullMutex_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&VxWorks64Mutex_Allocator, index) == true)
    {
        bool created = SolidSyslogVxWorks64Mutex_Initialise(&VxWorks64Mutex_Pool[index].Base);
        if (created == true)
        {
            handle = &VxWorks64Mutex_Pool[index].Base;
        }
        else
        {
            /* The kernel refused to make the semaphore, so the slot goes
             * straight back: the caller is handed the shared NullMutex and
             * holds nothing that names this slot. */
            (void) SolidSyslogPoolAllocator_FreeIfInUse(
                &VxWorks64Mutex_Allocator,
                index,
                VxWorks64Mutex_CleanupAtIndex,
                NULL
            );
        }
    }
    else
    {
        VxWorks64Mutex_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_MUTEX_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogVxWorks64Mutex_Destroy(struct SolidSyslogMutex* base)
{
    size_t index = VxWorks64Mutex_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&VxWorks64Mutex_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&VxWorks64Mutex_Allocator, index, VxWorks64Mutex_CleanupAtIndex, NULL);
    if (!released)
    {
        VxWorks64Mutex_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_MUTEX_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t VxWorks64Mutex_IndexFromHandle(const struct SolidSyslogMutex* base)
{
    size_t result = SOLIDSYSLOG_MUTEX_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_MUTEX_POOL_SIZE; poolIndex++)
    {
        if (base == &VxWorks64Mutex_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void VxWorks64Mutex_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogVxWorks64Mutex_Cleanup(&VxWorks64Mutex_Pool[index].Base);
}
