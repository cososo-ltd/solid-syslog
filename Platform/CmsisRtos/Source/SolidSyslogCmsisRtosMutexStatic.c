/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogCmsisRtosMutex.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogCmsisRtosMutexErrors.h"
#include "SolidSyslogCmsisRtosMutexPrivate.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogMutex;

static inline size_t CmsisRtosMutex_IndexFromHandle(const struct SolidSyslogMutex* base);
static inline void CmsisRtosMutex_CleanupAtIndex(size_t index, void* context);

static bool CmsisRtosMutex_InUse[SOLIDSYSLOG_MUTEX_POOL_SIZE];
static struct SolidSyslogCmsisRtosMutex CmsisRtosMutex_Pool[SOLIDSYSLOG_MUTEX_POOL_SIZE];
static struct SolidSyslogPoolAllocator CmsisRtosMutex_Allocator = {CmsisRtosMutex_InUse, SOLIDSYSLOG_MUTEX_POOL_SIZE};

struct SolidSyslogMutex* SolidSyslogCmsisRtosMutex_Create(void* controlBlock, uint32_t controlBlockBytes)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&CmsisRtosMutex_Allocator);
    struct SolidSyslogMutex* handle = SolidSyslogNullMutex_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&CmsisRtosMutex_Allocator, index) == true)
    {
        bool created =
            SolidSyslogCmsisRtosMutex_Initialise(&CmsisRtosMutex_Pool[index].Base, controlBlock, controlBlockBytes);
        if (created == true)
        {
            handle = &CmsisRtosMutex_Pool[index].Base;
        }
        else
        {
            /* The RTOS refused to make the mutex, so the slot goes straight
             * back: the caller is handed the shared NullMutex and holds nothing
             * that names this slot, and a deterministic refusal would otherwise
             * empty the pool one retry at a time. */
            (void) SolidSyslogPoolAllocator_FreeIfInUse(
                &CmsisRtosMutex_Allocator,
                index,
                CmsisRtosMutex_CleanupAtIndex,
                NULL
            );
        }
    }
    else
    {
        CmsisRtosMutex_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_MUTEX_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogCmsisRtosMutex_Destroy(struct SolidSyslogMutex* base)
{
    size_t index = CmsisRtosMutex_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&CmsisRtosMutex_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&CmsisRtosMutex_Allocator, index, CmsisRtosMutex_CleanupAtIndex, NULL);
    if (!released)
    {
        CmsisRtosMutex_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_MUTEX_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t CmsisRtosMutex_IndexFromHandle(const struct SolidSyslogMutex* base)
{
    size_t result = SOLIDSYSLOG_MUTEX_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_MUTEX_POOL_SIZE; poolIndex++)
    {
        if (base == &CmsisRtosMutex_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void CmsisRtosMutex_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogCmsisRtosMutex_Cleanup(&CmsisRtosMutex_Pool[index].Base);
}
