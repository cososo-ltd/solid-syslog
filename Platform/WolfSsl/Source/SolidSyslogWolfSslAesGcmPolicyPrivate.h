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

static inline void WolfSslAesGcmPolicy_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogAesGcmPolicyErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogWolfSslAesGcmPolicyErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGWOLFSSLAESGCMPOLICYPRIVATE_H */
