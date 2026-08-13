/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The RFC 5424 facility and severity enums that compose a message's PRIVAL. */
#ifndef SOLIDSYSLOGPRIVAL_H
#define SOLIDSYSLOGPRIVAL_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** RFC 5424 facility codes; the numeric values are the RFC's own. */
    enum SolidSyslogFacility
    {
        SOLIDSYSLOG_FACILITY_KERN = 0,
        SOLIDSYSLOG_FACILITY_USER = 1,
        SOLIDSYSLOG_FACILITY_MAIL = 2,
        SOLIDSYSLOG_FACILITY_DAEMON = 3,
        SOLIDSYSLOG_FACILITY_AUTH = 4,
        SOLIDSYSLOG_FACILITY_SYSLOG = 5,
        SOLIDSYSLOG_FACILITY_LPR = 6,
        SOLIDSYSLOG_FACILITY_NEWS = 7,
        SOLIDSYSLOG_FACILITY_UUCP = 8,
        SOLIDSYSLOG_FACILITY_CRON = 9,
        SOLIDSYSLOG_FACILITY_AUTH_PRIV = 10,
        SOLIDSYSLOG_FACILITY_FTP = 11,
        SOLIDSYSLOG_FACILITY_NTP = 12,
        SOLIDSYSLOG_FACILITY_AUDIT = 13,
        SOLIDSYSLOG_FACILITY_ALERT = 14,
        SOLIDSYSLOG_FACILITY_CLOCK = 15,
        SOLIDSYSLOG_FACILITY_LOCAL0 = 16,
        SOLIDSYSLOG_FACILITY_LOCAL1 = 17,
        SOLIDSYSLOG_FACILITY_LOCAL2 = 18,
        SOLIDSYSLOG_FACILITY_LOCAL3 = 19,
        SOLIDSYSLOG_FACILITY_LOCAL4 = 20,
        SOLIDSYSLOG_FACILITY_LOCAL5 = 21,
        SOLIDSYSLOG_FACILITY_LOCAL6 = 22,
        SOLIDSYSLOG_FACILITY_LOCAL7 = 23
    };

    /** RFC 5424 severities. When emitted on a SolidSyslog error event the level
     *  is an urgency axis (how bad is this now), not a who-fixes-it axis; see
     *  docs/error-severity.md for the policy each emit site follows. EMERGENCY,
     *  ALERT, INFORMATIONAL and DEBUG are reserved (not emitted by the library
     *  today). */
    enum SolidSyslogSeverity
    {
        SOLIDSYSLOG_SEVERITY_EMERGENCY = 0,
        SOLIDSYSLOG_SEVERITY_ALERT = 1,
        SOLIDSYSLOG_SEVERITY_CRITICAL = 2,
        SOLIDSYSLOG_SEVERITY_ERROR = 3,
        SOLIDSYSLOG_SEVERITY_WARNING = 4,
        SOLIDSYSLOG_SEVERITY_NOTICE = 5,
        SOLIDSYSLOG_SEVERITY_INFORMATIONAL = 6,
        SOLIDSYSLOG_SEVERITY_DEBUG = 7
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPRIVAL_H */
