/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGTIMEQUALITYSDPRIVATE_H
#define SOLIDSYSLOGTIMEQUALITYSDPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStructuredDataDefinition.h"
#include "SolidSyslogTimeQuality.h"
#include "SolidSyslogTimeQualitySdErrors.h"

struct SolidSyslogTimeQualitySd
{
    struct SolidSyslogStructuredData Base;
    SolidSyslogTimeQualityFunction GetTimeQuality;
};

void SolidSyslogTimeQualitySd_Initialise(
    struct SolidSyslogStructuredData* base,
    SolidSyslogTimeQualityFunction getTimeQuality
);
void SolidSyslogTimeQualitySd_Cleanup(struct SolidSyslogStructuredData* base);

static inline void TimeQualitySd_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogTimeQualitySdErrors code
)
{
    SolidSyslog_Error(severity, &TimeQualitySdErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGTIMEQUALITYSDPRIVATE_H */
