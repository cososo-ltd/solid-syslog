/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPrival.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const SeverityName_Names[] = {
    "EMERGENCY",
    "ALERT",
    "CRITICAL",
    "ERROR",
    "WARNING",
    "NOTICE",
    "INFORMATIONAL",
    "DEBUG",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(SeverityName_Names) / sizeof(SeverityName_Names[0])) == (SOLIDSYSLOG_SEVERITY_DEBUG + 1),
    every_severity_has_a_name
);

const char* SolidSyslogSeverity_Name(int32_t severity)
{
    return SolidSyslogErrorName_Lookup(SeverityName_Names, (int32_t) SOLIDSYSLOG_SEVERITY_DEBUG + 1, severity);
}
