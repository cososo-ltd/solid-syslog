/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPrival.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const SeverityName_Names[] = {
    [SOLIDSYSLOG_SEVERITY_EMERGENCY] = "EMERGENCY",
    [SOLIDSYSLOG_SEVERITY_ALERT] = "ALERT",
    [SOLIDSYSLOG_SEVERITY_CRITICAL] = "CRITICAL",
    [SOLIDSYSLOG_SEVERITY_ERROR] = "ERROR",
    [SOLIDSYSLOG_SEVERITY_WARNING] = "WARNING",
    [SOLIDSYSLOG_SEVERITY_NOTICE] = "NOTICE",
    [SOLIDSYSLOG_SEVERITY_INFORMATIONAL] = "INFORMATIONAL",
    [SOLIDSYSLOG_SEVERITY_DEBUG] = "DEBUG",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(SeverityName_Names) / sizeof(SeverityName_Names[0])) == ((size_t) SOLIDSYSLOG_SEVERITY_DEBUG + 1U),
    every_severity_has_a_name
);

const char* SolidSyslogSeverity_Name(int32_t severity)
{
    return SolidSyslogErrorName_Lookup(SeverityName_Names, (int32_t) SOLIDSYSLOG_SEVERITY_DEBUG + 1, severity);
}
