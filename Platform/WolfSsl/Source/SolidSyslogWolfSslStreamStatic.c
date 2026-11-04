/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWolfSslSettings.h"

#include "SolidSyslogWolfSslStream.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWolfSslStreamErrors.h"
#include "SolidSyslogWolfSslStreamPrivate.h"

struct SolidSyslogStream;

static inline bool WolfSslStream_IsValidConfig(const struct SolidSyslogWolfSslStreamConfig* config);
static inline size_t WolfSslStream_IndexFromHandle(const struct SolidSyslogStream* base);
static inline void WolfSslStream_CleanupAtIndex(size_t index, void* context);

static bool WolfSslStream_InUse[SOLIDSYSLOG_TLS_STREAM_POOL_SIZE];
static struct SolidSyslogWolfSslStream WolfSslStream_Pool[SOLIDSYSLOG_TLS_STREAM_POOL_SIZE];
static struct SolidSyslogPoolAllocator WolfSslStream_Allocator = {
    WolfSslStream_InUse,
    SOLIDSYSLOG_TLS_STREAM_POOL_SIZE
};

struct SolidSyslogStream* SolidSyslogWolfSslStream_Create(const struct SolidSyslogWolfSslStreamConfig* config)
{
    struct SolidSyslogStream* handle = SolidSyslogNullStream_Get();
    if (WolfSslStream_IsValidConfig(config))
    {
        size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&WolfSslStream_Allocator);
        if (SolidSyslogPoolAllocator_IndexIsValid(&WolfSslStream_Allocator, index) == true)
        {
            SolidSyslogWolfSslStream_Initialise(&WolfSslStream_Pool[index].Base, config);
            handle = &WolfSslStream_Pool[index].Base;
        }
        else
        {
            WolfSslStream_Report(
                SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
                SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
                SOLIDSYSLOG_TLS_STREAM_ERROR_POOL_EXHAUSTED
            );
        }
    }
    return handle;
}

static inline bool WolfSslStream_IsValidConfig(const struct SolidSyslogWolfSslStreamConfig* config)
{
    bool valid = false;
    if (config == NULL)
    {
        WolfSslStream_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_TLS_STREAM_ERROR_NULL_CONFIG
        );
    }
    else if (config->Transport == NULL)
    {
        WolfSslStream_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_TLS_STREAM_ERROR_NULL_TRANSPORT
        );
    }
    else if (config->Sleep == NULL)
    {
        WolfSslStream_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_TLS_STREAM_ERROR_NULL_SLEEP
        );
    }
    else if (config->Credentials == NULL)
    {
        WolfSslStream_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_TLS_STREAM_ERROR_NULL_CREDENTIALS
        );
    }
    else
    {
        valid = true;
    }
    return valid;
}

void SolidSyslogWolfSslStream_Destroy(struct SolidSyslogStream* base)
{
    size_t index = WolfSslStream_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&WolfSslStream_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&WolfSslStream_Allocator, index, WolfSslStream_CleanupAtIndex, NULL);
    if (!released)
    {
        WolfSslStream_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_TLS_STREAM_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t WolfSslStream_IndexFromHandle(const struct SolidSyslogStream* base)
{
    size_t result = SOLIDSYSLOG_TLS_STREAM_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_TLS_STREAM_POOL_SIZE; poolIndex++)
    {
        if (base == &WolfSslStream_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void WolfSslStream_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogWolfSslStream_Cleanup(&WolfSslStream_Pool[index].Base);
}
