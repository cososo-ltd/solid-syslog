/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

#if LWIP_SOCKET && LWIP_DNS

#include "SolidSyslogLwipSocketResolver.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogLwipSocketResolverErrors.h"
#include "SolidSyslogLwipSocketResolverPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogResolver;

static inline size_t LwipSocketResolver_IndexFromHandle(const struct SolidSyslogResolver* base);
static inline void LwipSocketResolver_CleanupAtIndex(size_t index, void* context);

static bool LwipSocketResolver_InUse[SOLIDSYSLOG_RESOLVER_POOL_SIZE];
static struct SolidSyslogLwipSocketResolver LwipSocketResolver_Pool[SOLIDSYSLOG_RESOLVER_POOL_SIZE];
static struct SolidSyslogPoolAllocator LwipSocketResolver_Allocator = {
    LwipSocketResolver_InUse,
    SOLIDSYSLOG_RESOLVER_POOL_SIZE
};

struct SolidSyslogResolver* SolidSyslogLwipSocketResolver_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&LwipSocketResolver_Allocator);
    struct SolidSyslogResolver* handle = SolidSyslogNullResolver_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&LwipSocketResolver_Allocator, index) == true)
    {
        SolidSyslogLwipSocketResolver_Initialise(&LwipSocketResolver_Pool[index].Base);
        handle = &LwipSocketResolver_Pool[index].Base;
    }
    else
    {
        LwipSocketResolver_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_RESOLVER_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogLwipSocketResolver_Destroy(struct SolidSyslogResolver* base)
{
    size_t index = LwipSocketResolver_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&LwipSocketResolver_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &LwipSocketResolver_Allocator,
                        index,
                        LwipSocketResolver_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        LwipSocketResolver_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_RESOLVER_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t LwipSocketResolver_IndexFromHandle(const struct SolidSyslogResolver* base)
{
    size_t result = SOLIDSYSLOG_RESOLVER_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_RESOLVER_POOL_SIZE; poolIndex++)
    {
        if (base == &LwipSocketResolver_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void LwipSocketResolver_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogLwipSocketResolver_Cleanup(&LwipSocketResolver_Pool[index].Base);
}

#else

/* ISO C forbids an empty translation unit. */
typedef int LwipSocketResolverStatic_EmptyTranslationUnit;

#endif /* LWIP_SOCKET && LWIP_DNS */
