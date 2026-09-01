/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGPOSIXRESOLVERPRIVATE_H
#define SOLIDSYSLOGPOSIXRESOLVERPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPosixResolverErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolverDefinition.h"

struct SolidSyslogPosixResolver
{
    struct SolidSyslogResolver Base;
};

void SolidSyslogPosixResolver_Initialise(struct SolidSyslogResolver* base);
void SolidSyslogPosixResolver_Cleanup(struct SolidSyslogResolver* base);

static inline void PosixResolver_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogPosixResolverErrors code
)
{
    SolidSyslog_Error(severity, &PosixResolverErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGPOSIXRESOLVERPRIVATE_H */
