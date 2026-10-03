/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Address.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogVxWorks64AddressErrors.h"
#include "SolidSyslogVxWorks64AddressPrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogAddress;

static inline struct SolidSyslogAddress* VxWorks64Address_HandleFromIndex(size_t index);
static inline size_t VxWorks64Address_IndexFromHandle(const struct SolidSyslogAddress* base);
static inline void VxWorks64Address_CleanupAtIndex(size_t index, void* context);

static bool VxWorks64Address_InUse[SOLIDSYSLOG_ADDRESS_POOL_SIZE];
static struct SolidSyslogPoolAllocator VxWorks64Address_Allocator = {
    VxWorks64Address_InUse,
    SOLIDSYSLOG_ADDRESS_POOL_SIZE
};

struct SolidSyslogAddress* SolidSyslogVxWorks64Address_Create(void)
{
    /* TU-private fallback returned when the pool is exhausted. Sized as
     * a real SolidSyslogVxWorks64Address so a Resolver overwrite at the
     * exhausted-fallback call site is bounded - same sockaddr_in storage
     * as any pooled slot. Not a per-Sender slot: multi-overflow integrators
     * share this storage and race on it. Bumping SOLIDSYSLOG_ADDRESS_POOL_SIZE
     * removes the race. */
    static struct SolidSyslogVxWorks64Address fallback;

    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&VxWorks64Address_Allocator);
    struct SolidSyslogAddress* handle = (struct SolidSyslogAddress*) &fallback;
    if (SolidSyslogPoolAllocator_IndexIsValid(&VxWorks64Address_Allocator, index) == true)
    {
        handle = VxWorks64Address_HandleFromIndex(index);
        SolidSyslogVxWorks64Address_Initialise(handle);
    }
    else
    {
        VxWorks64Address_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_ADDRESS_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

static inline struct SolidSyslogAddress* VxWorks64Address_HandleFromIndex(size_t index)
{
    static struct SolidSyslogVxWorks64Address pool[SOLIDSYSLOG_ADDRESS_POOL_SIZE];
    return (struct SolidSyslogAddress*) &pool[index];
}

void SolidSyslogVxWorks64Address_Destroy(struct SolidSyslogAddress* base)
{
    size_t index = VxWorks64Address_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&VxWorks64Address_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&VxWorks64Address_Allocator, index, VxWorks64Address_CleanupAtIndex, NULL);
    if (!released)
    {
        VxWorks64Address_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_ADDRESS_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t VxWorks64Address_IndexFromHandle(const struct SolidSyslogAddress* base)
{
    size_t result = SOLIDSYSLOG_ADDRESS_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_ADDRESS_POOL_SIZE; poolIndex++)
    {
        if (base == VxWorks64Address_HandleFromIndex(poolIndex))
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void VxWorks64Address_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogVxWorks64Address_Cleanup(VxWorks64Address_HandleFromIndex(index));
}
