/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPrival.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"

/* Sized by the ladder and indexed by its levels, so no name can shift. */
const char* SolidSyslogSeverity_Name(int32_t severity)
{
    static const char* const names[(int32_t) SOLIDSYSLOG_SEVERITY_DEBUG + 1] = {
        [SOLIDSYSLOG_SEVERITY_EMERGENCY] = "EMERGENCY",
        [SOLIDSYSLOG_SEVERITY_ALERT] = "ALERT",
        [SOLIDSYSLOG_SEVERITY_CRITICAL] = "CRITICAL",
        [SOLIDSYSLOG_SEVERITY_ERROR] = "ERROR",
        [SOLIDSYSLOG_SEVERITY_WARNING] = "WARNING",
        [SOLIDSYSLOG_SEVERITY_NOTICE] = "NOTICE",
        [SOLIDSYSLOG_SEVERITY_INFORMATIONAL] = "INFORMATIONAL",
        [SOLIDSYSLOG_SEVERITY_DEBUG] = "DEBUG",
    };

    return SolidSyslogErrorName_Lookup(names, (int32_t) SOLIDSYSLOG_SEVERITY_DEBUG + 1, severity);
}
