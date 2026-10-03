/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGVXWORKS64RESOLVERPRIVATE_H
#define SOLIDSYSLOGVXWORKS64RESOLVERPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogVxWorks64ResolverErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolverDefinition.h"

struct SolidSyslogVxWorks64Resolver
{
    struct SolidSyslogResolver Base;
};

void SolidSyslogVxWorks64Resolver_Initialise(struct SolidSyslogResolver* base);
void SolidSyslogVxWorks64Resolver_Cleanup(struct SolidSyslogResolver* base);

static inline void VxWorks64Resolver_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogResolverErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogVxWorks64ResolverErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGVXWORKS64RESOLVERPRIVATE_H */
