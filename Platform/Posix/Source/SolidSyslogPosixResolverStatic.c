/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPosixResolver.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPosixResolverErrors.h"
#include "SolidSyslogPosixResolverPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogResolver;

static inline size_t PosixResolver_IndexFromHandle(const struct SolidSyslogResolver* base);
static inline void PosixResolver_CleanupAtIndex(size_t index, void* context);

static bool PosixResolver_InUse[SOLIDSYSLOG_RESOLVER_POOL_SIZE];
static struct SolidSyslogPosixResolver PosixResolver_Pool[SOLIDSYSLOG_RESOLVER_POOL_SIZE];
static struct SolidSyslogPoolAllocator PosixResolver_Allocator = {PosixResolver_InUse, SOLIDSYSLOG_RESOLVER_POOL_SIZE};

struct SolidSyslogResolver* SolidSyslogPosixResolver_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&PosixResolver_Allocator);
    struct SolidSyslogResolver* handle = SolidSyslogNullResolver_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&PosixResolver_Allocator, index) == true)
    {
        PosixResolver_Initialise(&PosixResolver_Pool[index].Base);
        handle = &PosixResolver_Pool[index].Base;
    }
    else
    {
        PosixResolver_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_POSIX_RESOLVER_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogPosixResolver_Destroy(struct SolidSyslogResolver* base)
{
    size_t index = PosixResolver_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&PosixResolver_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&PosixResolver_Allocator, index, PosixResolver_CleanupAtIndex, NULL);
    if (!released)
    {
        PosixResolver_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_POSIX_RESOLVER_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t PosixResolver_IndexFromHandle(const struct SolidSyslogResolver* base)
{
    size_t result = SOLIDSYSLOG_RESOLVER_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_RESOLVER_POOL_SIZE; poolIndex++)
    {
        if (base == &PosixResolver_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void PosixResolver_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    PosixResolver_Cleanup(&PosixResolver_Pool[index].Base);
}
