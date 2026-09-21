/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

#if LWIP_SOCKET && LWIP_TCP

#include "SolidSyslogLwipSocketTcpStream.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogLwipSocketTcpStreamErrors.h"
#include "SolidSyslogLwipSocketTcpStreamPrivate.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogStream;

static inline size_t LwipSocketTcpStream_IndexFromHandle(const struct SolidSyslogStream* base);
static inline void LwipSocketTcpStream_CleanupAtIndex(size_t index, void* context);

static bool LwipSocketTcpStream_InUse[SOLIDSYSLOG_TCP_STREAM_POOL_SIZE];
static struct SolidSyslogLwipSocketTcpStream LwipSocketTcpStream_Pool[SOLIDSYSLOG_TCP_STREAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator LwipSocketTcpStream_Allocator = {
    LwipSocketTcpStream_InUse,
    SOLIDSYSLOG_TCP_STREAM_POOL_SIZE
};

struct SolidSyslogStream* SolidSyslogLwipSocketTcpStream_Create(
    const struct SolidSyslogLwipSocketTcpStreamConfig* config
)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&LwipSocketTcpStream_Allocator);
    struct SolidSyslogStream* handle = SolidSyslogNullStream_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&LwipSocketTcpStream_Allocator, index) == true)
    {
        SolidSyslogLwipSocketTcpStream_Initialise(&LwipSocketTcpStream_Pool[index].Base, config);
        handle = &LwipSocketTcpStream_Pool[index].Base;
    }
    else
    {
        LwipSocketTcpStream_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_TCP_STREAM_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogLwipSocketTcpStream_Destroy(struct SolidSyslogStream* base)
{
    size_t index = LwipSocketTcpStream_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&LwipSocketTcpStream_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &LwipSocketTcpStream_Allocator,
                        index,
                        LwipSocketTcpStream_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        LwipSocketTcpStream_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t LwipSocketTcpStream_IndexFromHandle(const struct SolidSyslogStream* base)
{
    size_t result = SOLIDSYSLOG_TCP_STREAM_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_TCP_STREAM_POOL_SIZE; poolIndex++)
    {
        if (base == &LwipSocketTcpStream_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void LwipSocketTcpStream_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogLwipSocketTcpStream_Cleanup(&LwipSocketTcpStream_Pool[index].Base);
}

#else

/* ISO C forbids an empty translation unit. */
typedef int LwipSocketTcpStreamStatic_EmptyTranslationUnit;

#endif /* LWIP_SOCKET && LWIP_TCP */
