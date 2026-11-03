/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWolfSslHmacSha256Policy.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogWolfSslHmacSha256PolicyErrors.h"
#include "SolidSyslogWolfSslHmacSha256PolicyPrivate.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslogTunables.h"

static inline bool WolfSslHmacSha256Policy_ConfigIsValid(const struct SolidSyslogWolfSslHmacSha256PolicyConfig* config);
static inline size_t WolfSslHmacSha256Policy_IndexFromHandle(const struct SolidSyslogSecurityPolicy* base);
static inline void WolfSslHmacSha256Policy_CleanupAtIndex(size_t index, void* context);

static bool WolfSslHmacSha256Policy_InUse[SOLIDSYSLOG_HMAC_SHA256_POLICY_POOL_SIZE];
static struct SolidSyslogWolfSslHmacSha256Policy WolfSslHmacSha256Policy_Pool[SOLIDSYSLOG_HMAC_SHA256_POLICY_POOL_SIZE];
static struct SolidSyslogPoolAllocator WolfSslHmacSha256Policy_Allocator = {
    WolfSslHmacSha256Policy_InUse,
    SOLIDSYSLOG_HMAC_SHA256_POLICY_POOL_SIZE
};

struct SolidSyslogSecurityPolicy* SolidSyslogWolfSslHmacSha256Policy_Create(
    const struct SolidSyslogWolfSslHmacSha256PolicyConfig* config
)
{
    struct SolidSyslogSecurityPolicy* handle = SolidSyslogNullSecurityPolicy_Get();
    if (WolfSslHmacSha256Policy_ConfigIsValid(config))
    {
        size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&WolfSslHmacSha256Policy_Allocator);
        if (SolidSyslogPoolAllocator_IndexIsValid(&WolfSslHmacSha256Policy_Allocator, index) == true)
        {
            SolidSyslogWolfSslHmacSha256Policy_Initialise(&WolfSslHmacSha256Policy_Pool[index].Base, config);
            handle = &WolfSslHmacSha256Policy_Pool[index].Base;
        }
        else
        {
            WolfSslHmacSha256Policy_Report(
                SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
                SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
                SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_POOL_EXHAUSTED
            );
        }
    }
    else
    {
        WolfSslHmacSha256Policy_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_BAD_CONFIG
        );
    }
    return handle;
}

void SolidSyslogWolfSslHmacSha256Policy_Destroy(struct SolidSyslogSecurityPolicy* base)
{
    size_t index = WolfSslHmacSha256Policy_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&WolfSslHmacSha256Policy_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &WolfSslHmacSha256Policy_Allocator,
                        index,
                        WolfSslHmacSha256Policy_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        WolfSslHmacSha256Policy_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline bool WolfSslHmacSha256Policy_ConfigIsValid(const struct SolidSyslogWolfSslHmacSha256PolicyConfig* config)
{
    return (config != NULL) && (config->GetKey != NULL);
}

static inline size_t WolfSslHmacSha256Policy_IndexFromHandle(const struct SolidSyslogSecurityPolicy* base)
{
    size_t result = SOLIDSYSLOG_HMAC_SHA256_POLICY_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_HMAC_SHA256_POLICY_POOL_SIZE; poolIndex++)
    {
        if (base == &WolfSslHmacSha256Policy_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void WolfSslHmacSha256Policy_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogWolfSslHmacSha256Policy_Cleanup(&WolfSslHmacSha256Policy_Pool[index].Base);
}
