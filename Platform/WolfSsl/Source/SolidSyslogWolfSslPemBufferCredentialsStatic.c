/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWolfSslPemBufferCredentials.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWolfSslCredentialsDefinition.h"
#include "SolidSyslogWolfSslNullCredentials.h"
#include "SolidSyslogWolfSslPemBufferCredentialsPrivate.h"

static inline bool WolfSslPemBufferCredentials_IsValidConfig(
    const struct SolidSyslogWolfSslPemBufferCredentialsConfig* config
);
static inline size_t WolfSslPemBufferCredentials_IndexFromHandle(const struct SolidSyslogWolfSslCredentials* base);
static inline void WolfSslPemBufferCredentials_CleanupAtIndex(size_t index, void* context);

static bool WolfSslPemBufferCredentials_InUse[SOLIDSYSLOG_TLS_CREDENTIALS_POOL_SIZE];
static struct SolidSyslogWolfSslPemBufferCredentials
    WolfSslPemBufferCredentials_Pool[SOLIDSYSLOG_TLS_CREDENTIALS_POOL_SIZE];
static struct SolidSyslogPoolAllocator WolfSslPemBufferCredentials_Allocator = {
    WolfSslPemBufferCredentials_InUse,
    SOLIDSYSLOG_TLS_CREDENTIALS_POOL_SIZE
};

struct SolidSyslogWolfSslCredentials* SolidSyslogWolfSslPemBufferCredentials_Create(
    const struct SolidSyslogWolfSslPemBufferCredentialsConfig* config
)
{
    struct SolidSyslogWolfSslCredentials* handle = SolidSyslogWolfSslNullCredentials_Get();
    if (WolfSslPemBufferCredentials_IsValidConfig(config))
    {
        size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&WolfSslPemBufferCredentials_Allocator);
        if (SolidSyslogPoolAllocator_IndexIsValid(&WolfSslPemBufferCredentials_Allocator, index) == true)
        {
            SolidSyslogWolfSslPemBufferCredentials_Initialise(&WolfSslPemBufferCredentials_Pool[index].Base, config);
            handle = &WolfSslPemBufferCredentials_Pool[index].Base;
        }
        else
        {
            WolfSslPemBufferCredentials_Report(
                SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
                SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
                SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_POOL_EXHAUSTED
            );
        }
    }
    return handle;
}

static inline bool WolfSslPemBufferCredentials_IsValidConfig(
    const struct SolidSyslogWolfSslPemBufferCredentialsConfig* config
)
{
    bool valid = false;
    if (config == NULL)
    {
        WolfSslPemBufferCredentials_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_NULL_CONFIG
        );
    }
    else
    {
        valid = true;
    }
    return valid;
}

void SolidSyslogWolfSslPemBufferCredentials_Destroy(struct SolidSyslogWolfSslCredentials* base)
{
    size_t index = WolfSslPemBufferCredentials_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&WolfSslPemBufferCredentials_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &WolfSslPemBufferCredentials_Allocator,
                        index,
                        WolfSslPemBufferCredentials_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        WolfSslPemBufferCredentials_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t WolfSslPemBufferCredentials_IndexFromHandle(const struct SolidSyslogWolfSslCredentials* base)
{
    size_t result = SOLIDSYSLOG_TLS_CREDENTIALS_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_TLS_CREDENTIALS_POOL_SIZE; poolIndex++)
    {
        if (base == &WolfSslPemBufferCredentials_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void WolfSslPemBufferCredentials_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogWolfSslPemBufferCredentials_Cleanup(&WolfSslPemBufferCredentials_Pool[index].Base);
}
