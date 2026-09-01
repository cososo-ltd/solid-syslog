/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGORIGINSDPRIVATE_H
#define SOLIDSYSLOGORIGINSDPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogOriginSd.h"
#include "SolidSyslogOriginSdErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStructuredDataDefinition.h"

enum
{
    ORIGIN_SOFTWARE_MAX = 48,
    ORIGIN_SWVERSION_MAX = 32,
    ORIGIN_ENTERPRISE_ID_MAX = 64
};

struct SolidSyslogOriginSd
{
    struct SolidSyslogStructuredData Base;
    const char* Software;
    const char* SwVersion;
    const char* EnterpriseId;
    SolidSyslogOriginIpCountFunction GetIpCount;
    SolidSyslogOriginIpAtFunction GetIpAt;
    void* IpContext;
};

void SolidSyslogOriginSd_Initialise(
    struct SolidSyslogStructuredData* base,
    const struct SolidSyslogOriginSdConfig* config
);
void SolidSyslogOriginSd_Cleanup(struct SolidSyslogStructuredData* base);

static inline void OriginSd_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogOriginSdErrors code
)
{
    SolidSyslog_Error(severity, &OriginSdErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGORIGINSDPRIVATE_H */
