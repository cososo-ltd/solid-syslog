/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

/* This component requires lwIP built with UDP. */
#if LWIP_UDP

#include "SolidSyslogLwipRawDatagram.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogLwipRawDatagramErrors.h"
#include "SolidSyslogLwipRawDatagramPrivate.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogDatagram;

static inline size_t LwipRawDatagram_IndexFromHandle(const struct SolidSyslogDatagram* base);
static inline void LwipRawDatagram_CleanupAtIndex(size_t index, void* context);

static bool LwipRawDatagram_InUse[SOLIDSYSLOG_DATAGRAM_POOL_SIZE];
static struct SolidSyslogLwipRawDatagram LwipRawDatagram_Pool[SOLIDSYSLOG_DATAGRAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator LwipRawDatagram_Allocator = {
    LwipRawDatagram_InUse,
    SOLIDSYSLOG_DATAGRAM_POOL_SIZE
};

struct SolidSyslogDatagram* SolidSyslogLwipRawDatagram_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&LwipRawDatagram_Allocator);
    struct SolidSyslogDatagram* handle = SolidSyslogNullDatagram_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&LwipRawDatagram_Allocator, index) == true)
    {
        SolidSyslogLwipRawDatagram_Initialise(&LwipRawDatagram_Pool[index].Base);
        handle = &LwipRawDatagram_Pool[index].Base;
    }
    else
    {
        LwipRawDatagram_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_LWIPRAW_DATAGRAM_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogLwipRawDatagram_Destroy(struct SolidSyslogDatagram* base)
{
    size_t index = LwipRawDatagram_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&LwipRawDatagram_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&LwipRawDatagram_Allocator, index, LwipRawDatagram_CleanupAtIndex, NULL);
    if (!released)
    {
        LwipRawDatagram_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_LWIPRAW_DATAGRAM_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t LwipRawDatagram_IndexFromHandle(const struct SolidSyslogDatagram* base)
{
    size_t result = SOLIDSYSLOG_DATAGRAM_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_DATAGRAM_POOL_SIZE; poolIndex++)
    {
        if (base == &LwipRawDatagram_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void LwipRawDatagram_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogLwipRawDatagram_Cleanup(&LwipRawDatagram_Pool[index].Base);
}

#else

/* ISO C forbids an empty translation unit. */
typedef int LwipRawDatagramStatic_EmptyTranslationUnit;

#endif /* LWIP_UDP */
