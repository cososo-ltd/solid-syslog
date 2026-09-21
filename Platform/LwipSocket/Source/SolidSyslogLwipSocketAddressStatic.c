/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

#if LWIP_SOCKET

#include "SolidSyslogLwipSocketAddress.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogLwipSocketAddressErrors.h"
#include "SolidSyslogLwipSocketAddressPrivate.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogAddress;

static inline struct SolidSyslogAddress* LwipSocketAddress_HandleFromIndex(size_t index);
static inline size_t LwipSocketAddress_IndexFromHandle(const struct SolidSyslogAddress* base);
static inline void LwipSocketAddress_CleanupAtIndex(size_t index, void* context);

static bool LwipSocketAddress_InUse[SOLIDSYSLOG_ADDRESS_POOL_SIZE];
static struct SolidSyslogPoolAllocator LwipSocketAddress_Allocator = {
    LwipSocketAddress_InUse,
    SOLIDSYSLOG_ADDRESS_POOL_SIZE
};

struct SolidSyslogAddress* SolidSyslogLwipSocketAddress_Create(void)
{
    /* TU-private fallback returned when the pool is exhausted. Sized as a real
     * SolidSyslogLwipSocketAddress so a Resolver overwrite at the exhausted-fallback
     * call site is bounded - same sockaddr_in storage as any pooled slot. Not a
     * per-Sender slot: multi-overflow integrators share this storage and race on it.
     * Bumping SOLIDSYSLOG_ADDRESS_POOL_SIZE removes the race. */
    static struct SolidSyslogLwipSocketAddress fallback;

    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&LwipSocketAddress_Allocator);
    struct SolidSyslogAddress* handle = (struct SolidSyslogAddress*) &fallback;
    if (SolidSyslogPoolAllocator_IndexIsValid(&LwipSocketAddress_Allocator, index) == true)
    {
        handle = LwipSocketAddress_HandleFromIndex(index);
        SolidSyslogLwipSocketAddress_Initialise(handle);
    }
    else
    {
        LwipSocketAddress_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_ADDRESS_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

static inline struct SolidSyslogAddress* LwipSocketAddress_HandleFromIndex(size_t index)
{
    static struct SolidSyslogLwipSocketAddress pool[SOLIDSYSLOG_ADDRESS_POOL_SIZE];
    return (struct SolidSyslogAddress*) &pool[index];
}

void SolidSyslogLwipSocketAddress_Destroy(struct SolidSyslogAddress* base)
{
    size_t index = LwipSocketAddress_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&LwipSocketAddress_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &LwipSocketAddress_Allocator,
                        index,
                        LwipSocketAddress_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        LwipSocketAddress_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_ADDRESS_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t LwipSocketAddress_IndexFromHandle(const struct SolidSyslogAddress* base)
{
    size_t result = SOLIDSYSLOG_ADDRESS_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_ADDRESS_POOL_SIZE; poolIndex++)
    {
        if (base == LwipSocketAddress_HandleFromIndex(poolIndex))
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void LwipSocketAddress_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogLwipSocketAddress_Cleanup(LwipSocketAddress_HandleFromIndex(index));
}

#else

/* ISO C forbids an empty translation unit. */
typedef int LwipSocketAddressStatic_EmptyTranslationUnit;

#endif /* LWIP_SOCKET */
