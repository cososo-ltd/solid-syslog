/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWolfSslAesGcmPolicy.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogWolfSslAesGcmPolicyErrors.h"
#include "SolidSyslogWolfSslAesGcmPolicyPrivate.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslogTunables.h"

static inline bool WolfSslAesGcmPolicy_ConfigIsValid(const struct SolidSyslogWolfSslAesGcmPolicyConfig* config);
static inline size_t WolfSslAesGcmPolicy_IndexFromHandle(const struct SolidSyslogSecurityPolicy* base);
static inline void WolfSslAesGcmPolicy_CleanupAtIndex(size_t index, void* context);

static bool WolfSslAesGcmPolicy_InUse[SOLIDSYSLOG_AES_GCM_POLICY_POOL_SIZE];
static struct SolidSyslogWolfSslAesGcmPolicy WolfSslAesGcmPolicy_Pool[SOLIDSYSLOG_AES_GCM_POLICY_POOL_SIZE];
static struct SolidSyslogPoolAllocator WolfSslAesGcmPolicy_Allocator = {
    WolfSslAesGcmPolicy_InUse,
    SOLIDSYSLOG_AES_GCM_POLICY_POOL_SIZE
};

struct SolidSyslogSecurityPolicy* SolidSyslogWolfSslAesGcmPolicy_Create(
    const struct SolidSyslogWolfSslAesGcmPolicyConfig* config
)
{
    struct SolidSyslogSecurityPolicy* handle = SolidSyslogNullSecurityPolicy_Get();
    if (WolfSslAesGcmPolicy_ConfigIsValid(config))
    {
        size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&WolfSslAesGcmPolicy_Allocator);
        if (SolidSyslogPoolAllocator_IndexIsValid(&WolfSslAesGcmPolicy_Allocator, index) == true)
        {
            SolidSyslogWolfSslAesGcmPolicy_Initialise(&WolfSslAesGcmPolicy_Pool[index].Base, config);
            handle = &WolfSslAesGcmPolicy_Pool[index].Base;
        }
        else
        {
            WolfSslAesGcmPolicy_Report(
                SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
                SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
                SOLIDSYSLOG_AES_GCM_POLICY_ERROR_POOL_EXHAUSTED
            );
        }
    }
    else
    {
        WolfSslAesGcmPolicy_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_AES_GCM_POLICY_ERROR_BAD_CONFIG
        );
    }
    return handle;
}

void SolidSyslogWolfSslAesGcmPolicy_Destroy(struct SolidSyslogSecurityPolicy* base)
{
    size_t index = WolfSslAesGcmPolicy_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&WolfSslAesGcmPolicy_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &WolfSslAesGcmPolicy_Allocator,
                        index,
                        WolfSslAesGcmPolicy_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        WolfSslAesGcmPolicy_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_AES_GCM_POLICY_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline bool WolfSslAesGcmPolicy_ConfigIsValid(const struct SolidSyslogWolfSslAesGcmPolicyConfig* config)
{
    return (config != NULL) && (config->GetKey != NULL) && (config->Rng != NULL);
}

static inline size_t WolfSslAesGcmPolicy_IndexFromHandle(const struct SolidSyslogSecurityPolicy* base)
{
    size_t result = SOLIDSYSLOG_AES_GCM_POLICY_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_AES_GCM_POLICY_POOL_SIZE; poolIndex++)
    {
        if (base == &WolfSslAesGcmPolicy_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void WolfSslAesGcmPolicy_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogWolfSslAesGcmPolicy_Cleanup(&WolfSslAesGcmPolicy_Pool[index].Base);
}
