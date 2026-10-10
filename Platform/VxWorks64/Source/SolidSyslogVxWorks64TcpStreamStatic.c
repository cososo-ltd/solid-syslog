/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64TcpStream.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogVxWorks64TcpStreamErrors.h"
#include "SolidSyslogVxWorks64TcpStreamPrivate.h"

static inline size_t VxWorks64TcpStream_IndexFromHandle(const struct SolidSyslogStream* base);
static inline void VxWorks64TcpStream_CleanupAtIndex(size_t index, void* context);

static bool VxWorks64TcpStream_InUse[SOLIDSYSLOG_TCP_STREAM_POOL_SIZE];
static struct SolidSyslogVxWorks64TcpStream VxWorks64TcpStream_Pool[SOLIDSYSLOG_TCP_STREAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator VxWorks64TcpStream_Allocator = {
    VxWorks64TcpStream_InUse,
    SOLIDSYSLOG_TCP_STREAM_POOL_SIZE
};

struct SolidSyslogStream* SolidSyslogVxWorks64TcpStream_Create(const struct SolidSyslogVxWorks64TcpStreamConfig* config)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&VxWorks64TcpStream_Allocator);
    struct SolidSyslogStream* handle = SolidSyslogNullStream_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&VxWorks64TcpStream_Allocator, index) == true)
    {
        SolidSyslogVxWorks64TcpStream_Initialise(&VxWorks64TcpStream_Pool[index].Base, config);
        handle = &VxWorks64TcpStream_Pool[index].Base;
    }
    else
    {
        VxWorks64TcpStream_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_TCP_STREAM_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogVxWorks64TcpStream_Destroy(struct SolidSyslogStream* base)
{
    size_t index = VxWorks64TcpStream_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&VxWorks64TcpStream_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &VxWorks64TcpStream_Allocator,
                        index,
                        VxWorks64TcpStream_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        VxWorks64TcpStream_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t VxWorks64TcpStream_IndexFromHandle(const struct SolidSyslogStream* base)
{
    size_t result = SOLIDSYSLOG_TCP_STREAM_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_TCP_STREAM_POOL_SIZE; poolIndex++)
    {
        if (base == &VxWorks64TcpStream_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void VxWorks64TcpStream_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogVxWorks64TcpStream_Cleanup(&VxWorks64TcpStream_Pool[index].Base);
}
