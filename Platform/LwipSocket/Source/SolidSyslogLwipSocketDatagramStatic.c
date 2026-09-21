/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

#if LWIP_SOCKET && LWIP_UDP

#include "SolidSyslogLwipSocketDatagram.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogLwipSocketDatagramErrors.h"
#include "SolidSyslogLwipSocketDatagramPrivate.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogDatagram;

static inline size_t LwipSocketDatagram_IndexFromHandle(const struct SolidSyslogDatagram* base);
static inline void LwipSocketDatagram_CleanupAtIndex(size_t index, void* context);

static bool LwipSocketDatagram_InUse[SOLIDSYSLOG_DATAGRAM_POOL_SIZE];
static struct SolidSyslogLwipSocketDatagram LwipSocketDatagram_Pool[SOLIDSYSLOG_DATAGRAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator LwipSocketDatagram_Allocator = {
    LwipSocketDatagram_InUse,
    SOLIDSYSLOG_DATAGRAM_POOL_SIZE
};

struct SolidSyslogDatagram* SolidSyslogLwipSocketDatagram_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&LwipSocketDatagram_Allocator);
    struct SolidSyslogDatagram* handle = SolidSyslogNullDatagram_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&LwipSocketDatagram_Allocator, index) == true)
    {
        SolidSyslogLwipSocketDatagram_Initialise(&LwipSocketDatagram_Pool[index].Base);
        handle = &LwipSocketDatagram_Pool[index].Base;
    }
    else
    {
        LwipSocketDatagram_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_DATAGRAM_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogLwipSocketDatagram_Destroy(struct SolidSyslogDatagram* base)
{
    size_t index = LwipSocketDatagram_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&LwipSocketDatagram_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &LwipSocketDatagram_Allocator,
                        index,
                        LwipSocketDatagram_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        LwipSocketDatagram_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_DATAGRAM_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t LwipSocketDatagram_IndexFromHandle(const struct SolidSyslogDatagram* base)
{
    size_t result = SOLIDSYSLOG_DATAGRAM_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_DATAGRAM_POOL_SIZE; poolIndex++)
    {
        if (base == &LwipSocketDatagram_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void LwipSocketDatagram_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogLwipSocketDatagram_Cleanup(&LwipSocketDatagram_Pool[index].Base);
}

#else

/* ISO C forbids an empty translation unit. */
typedef int LwipSocketDatagramStatic_EmptyTranslationUnit;

#endif /* LWIP_SOCKET && LWIP_UDP */
