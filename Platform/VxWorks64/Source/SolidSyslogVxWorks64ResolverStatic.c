/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Resolver.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogVxWorks64ResolverErrors.h"
#include "SolidSyslogVxWorks64ResolverPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogResolver;

static inline size_t VxWorks64Resolver_IndexFromHandle(const struct SolidSyslogResolver* base);
static inline void VxWorks64Resolver_CleanupAtIndex(size_t index, void* context);

static bool VxWorks64Resolver_InUse[SOLIDSYSLOG_RESOLVER_POOL_SIZE];
static struct SolidSyslogVxWorks64Resolver VxWorks64Resolver_Pool[SOLIDSYSLOG_RESOLVER_POOL_SIZE];
static struct SolidSyslogPoolAllocator VxWorks64Resolver_Allocator = {
    VxWorks64Resolver_InUse,
    SOLIDSYSLOG_RESOLVER_POOL_SIZE
};

struct SolidSyslogResolver* SolidSyslogVxWorks64Resolver_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&VxWorks64Resolver_Allocator);
    struct SolidSyslogResolver* handle = SolidSyslogNullResolver_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&VxWorks64Resolver_Allocator, index) == true)
    {
        SolidSyslogVxWorks64Resolver_Initialise(&VxWorks64Resolver_Pool[index].Base);
        handle = &VxWorks64Resolver_Pool[index].Base;
    }
    else
    {
        VxWorks64Resolver_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_RESOLVER_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogVxWorks64Resolver_Destroy(struct SolidSyslogResolver* base)
{
    size_t index = VxWorks64Resolver_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&VxWorks64Resolver_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &VxWorks64Resolver_Allocator,
                        index,
                        VxWorks64Resolver_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        VxWorks64Resolver_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_RESOLVER_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t VxWorks64Resolver_IndexFromHandle(const struct SolidSyslogResolver* base)
{
    size_t result = SOLIDSYSLOG_RESOLVER_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_RESOLVER_POOL_SIZE; poolIndex++)
    {
        if (base == &VxWorks64Resolver_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void VxWorks64Resolver_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogVxWorks64Resolver_Cleanup(&VxWorks64Resolver_Pool[index].Base);
}
