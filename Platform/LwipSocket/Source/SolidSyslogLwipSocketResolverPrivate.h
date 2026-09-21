/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGLWIPSOCKETRESOLVERPRIVATE_H
#define SOLIDSYSLOGLWIPSOCKETRESOLVERPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipSocketResolverErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolverDefinition.h"

struct SolidSyslogLwipSocketResolver
{
    struct SolidSyslogResolver Base;
};

void SolidSyslogLwipSocketResolver_Initialise(struct SolidSyslogResolver* base);
void SolidSyslogLwipSocketResolver_Cleanup(struct SolidSyslogResolver* base);

static inline void LwipSocketResolver_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogResolverErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogLwipSocketResolverErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGLWIPSOCKETRESOLVERPRIVATE_H */
