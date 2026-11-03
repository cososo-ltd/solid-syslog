/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGWOLFSSLAESGCMPOLICYPRIVATE_H
#define SOLIDSYSLOGWOLFSSLAESGCMPOLICYPRIVATE_H

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogWolfSslAesGcmPolicy.h"
#include "SolidSyslogWolfSslAesGcmPolicyErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyDefinition.h"

enum
{
    AES_GCM_POLICY_KEY_SIZE = 32,
    AES_GCM_POLICY_NONCE_SIZE = 12,
    AES_GCM_POLICY_TAG_SIZE = 16,
    /* Trailer is nonce || tag - fits SOLIDSYSLOG_MAX_INTEGRITY_SIZE (32). */
    AES_GCM_POLICY_TRAILER_SIZE = AES_GCM_POLICY_NONCE_SIZE + AES_GCM_POLICY_TAG_SIZE
};

struct SolidSyslogWolfSslAesGcmPolicy
{
    struct SolidSyslogSecurityPolicy Base;
    struct SolidSyslogWolfSslAesGcmPolicyConfig Config;
    bool FaultReported; /* A run of faults is reported once; a verdict re-arms it. */
};

void SolidSyslogWolfSslAesGcmPolicy_Initialise(
    struct SolidSyslogSecurityPolicy* base,
    const struct SolidSyslogWolfSslAesGcmPolicyConfig* config
);
void SolidSyslogWolfSslAesGcmPolicy_Cleanup(struct SolidSyslogSecurityPolicy* base);

/* The two wolfCrypt calls, in their own translation unit so that a build whose
 * wc_AesGcmEncrypt does not keep wolfCrypt's exact-size buffer contract leaves
 * them undefined rather than overrunning the record. See the platform page. */
bool SolidSyslogWolfSslAesGcmPolicy_GcmEncrypt(const struct SolidSyslogSecurityRecord* record, const uint8_t* key);
enum SolidSyslogSecurityPolicyOpenResult SolidSyslogWolfSslAesGcmPolicy_GcmDecrypt(
    const struct SolidSyslogSecurityRecord* record,
    const uint8_t* key
);

static inline void WolfSslAesGcmPolicy_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogAesGcmPolicyErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogWolfSslAesGcmPolicyErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGWOLFSSLAESGCMPOLICYPRIVATE_H */
