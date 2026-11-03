/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGWOLFSSLHMACSHA256POLICYPRIVATE_H
#define SOLIDSYSLOGWOLFSSLHMACSHA256POLICYPRIVATE_H

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslogWolfSslHmacSha256Policy.h"
#include "SolidSyslogWolfSslHmacSha256PolicyErrors.h"

struct SolidSyslogWolfSslHmacSha256Policy
{
    struct SolidSyslogSecurityPolicy Base;
    struct SolidSyslogWolfSslHmacSha256PolicyConfig Config;
    bool FaultReported; /* A run of faults is reported once; a verdict re-arms it. */
};

void SolidSyslogWolfSslHmacSha256Policy_Initialise(
    struct SolidSyslogSecurityPolicy* base,
    const struct SolidSyslogWolfSslHmacSha256PolicyConfig* config
);
void SolidSyslogWolfSslHmacSha256Policy_Cleanup(struct SolidSyslogSecurityPolicy* base);

static inline void WolfSslHmacSha256Policy_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogHmacSha256PolicyErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogWolfSslHmacSha256PolicyErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGWOLFSSLHMACSHA256POLICYPRIVATE_H */
