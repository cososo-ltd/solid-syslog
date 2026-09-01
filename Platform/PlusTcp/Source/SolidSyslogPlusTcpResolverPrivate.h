/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGPLUSTCPRESOLVERPRIVATE_H
#define SOLIDSYSLOGPLUSTCPRESOLVERPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPlusTcpResolverErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolverDefinition.h"

struct SolidSyslogPlusTcpResolver
{
    struct SolidSyslogResolver Base;
};

void SolidSyslogPlusTcpResolver_Initialise(struct SolidSyslogResolver* base);
void SolidSyslogPlusTcpResolver_Cleanup(struct SolidSyslogResolver* base);

static inline void PlusTcpResolver_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogPlusTcpResolverErrors code
)
{
    SolidSyslog_Error(severity, &PlusTcpResolverErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGPLUSTCPRESOLVERPRIVATE_H */
