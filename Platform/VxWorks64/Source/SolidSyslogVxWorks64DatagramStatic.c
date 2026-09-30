/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Datagram.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogVxWorks64DatagramErrors.h"
#include "SolidSyslogVxWorks64DatagramPrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogDatagram;

static inline size_t VxWorks64Datagram_IndexFromHandle(const struct SolidSyslogDatagram* base);
static inline void VxWorks64Datagram_CleanupAtIndex(size_t index, void* context);

static bool VxWorks64Datagram_InUse[SOLIDSYSLOG_DATAGRAM_POOL_SIZE];
static struct SolidSyslogVxWorks64Datagram VxWorks64Datagram_Pool[SOLIDSYSLOG_DATAGRAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator VxWorks64Datagram_Allocator = {
    VxWorks64Datagram_InUse,
    SOLIDSYSLOG_DATAGRAM_POOL_SIZE
};

struct SolidSyslogDatagram* SolidSyslogVxWorks64Datagram_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&VxWorks64Datagram_Allocator);
    struct SolidSyslogDatagram* handle = SolidSyslogNullDatagram_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&VxWorks64Datagram_Allocator, index) == true)
    {
        SolidSyslogVxWorks64Datagram_Initialise(&VxWorks64Datagram_Pool[index].Base);
        handle = &VxWorks64Datagram_Pool[index].Base;
    }
    else
    {
        VxWorks64Datagram_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_DATAGRAM_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogVxWorks64Datagram_Destroy(struct SolidSyslogDatagram* base)
{
    size_t index = VxWorks64Datagram_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&VxWorks64Datagram_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &VxWorks64Datagram_Allocator,
                        index,
                        VxWorks64Datagram_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        VxWorks64Datagram_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_DATAGRAM_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t VxWorks64Datagram_IndexFromHandle(const struct SolidSyslogDatagram* base)
{
    size_t result = SOLIDSYSLOG_DATAGRAM_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_DATAGRAM_POOL_SIZE; poolIndex++)
    {
        if (base == &VxWorks64Datagram_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void VxWorks64Datagram_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogVxWorks64Datagram_Cleanup(&VxWorks64Datagram_Pool[index].Base);
}
