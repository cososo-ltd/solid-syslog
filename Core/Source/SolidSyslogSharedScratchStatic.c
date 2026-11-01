/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogSharedScratch.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSharedScratchErrors.h"
#include "SolidSyslogSharedScratchPrivate.h"
#include "SolidSyslogStackScratch.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogMutex;
struct SolidSyslogScratch;

static inline size_t SharedScratch_IndexFromHandle(const struct SolidSyslogScratch* base);
static inline void SharedScratch_CleanupAtIndex(size_t index, void* context);

/* One per logger the build can hold: the logger pool tunable sizes this pool
 * too, and a build that never calls Create links none of it. */
static bool SharedScratch_InUse[SOLIDSYSLOG_POOL_SIZE];
static struct SolidSyslogSharedScratch SharedScratch_Pool[SOLIDSYSLOG_POOL_SIZE];
static struct SolidSyslogPoolAllocator SharedScratch_Allocator = {SharedScratch_InUse, SOLIDSYSLOG_POOL_SIZE};

/* Both faults fall back to the stack scratch, so logging goes on; the report
 * says the RAM saving is not being made. */
struct SolidSyslogScratch* SolidSyslogSharedScratch_Create(struct SolidSyslogMutex* mutex)
{
    struct SolidSyslogScratch* handle = SolidSyslogStackScratch_Get();
    if (mutex == NULL)
    {
        SharedScratch_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_SHARED_SCRATCH_ERROR_NULL_MUTEX
        );
    }
    else
    {
        size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&SharedScratch_Allocator);
        if (SolidSyslogPoolAllocator_IndexIsValid(&SharedScratch_Allocator, index))
        {
            SolidSyslogSharedScratch_Initialise(&SharedScratch_Pool[index].Base, mutex);
            handle = &SharedScratch_Pool[index].Base;
        }
        else
        {
            SharedScratch_Report(
                SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
                SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
                SOLIDSYSLOG_SHARED_SCRATCH_ERROR_POOL_EXHAUSTED
            );
        }
    }
    return handle;
}

void SolidSyslogSharedScratch_Destroy(struct SolidSyslogScratch* base)
{
    size_t index = SharedScratch_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&SharedScratch_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&SharedScratch_Allocator, index, SharedScratch_CleanupAtIndex, NULL);
    if (!released)
    {
        SharedScratch_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_SHARED_SCRATCH_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t SharedScratch_IndexFromHandle(const struct SolidSyslogScratch* base)
{
    size_t result = SOLIDSYSLOG_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_POOL_SIZE; poolIndex++)
    {
        if (base == &SharedScratch_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void SharedScratch_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogSharedScratch_Cleanup(&SharedScratch_Pool[index].Base);
}
