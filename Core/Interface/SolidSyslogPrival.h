#ifndef SOLIDSYSLOGPRIVAL_H
#define SOLIDSYSLOGPRIVAL_H

#include "ExternC.h"

EXTERN_C_BEGIN

    enum SolidSyslog_Facility
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
        SOLIDSYSLOG_FACILITY_AUTHPRIV = 10,
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

    enum SolidSyslogSeverity
    {
        SolidSyslogSeverity_Emergency = 0,
        SolidSyslogSeverity_Alert = 1,
        SolidSyslogSeverity_Critical = 2,
        SolidSyslogSeverity_Error = 3,
        SolidSyslogSeverity_Warning = 4,
        SolidSyslogSeverity_Notice = 5,
        SolidSyslogSeverity_Informational = 6,
        SolidSyslogSeverity_Debug = 7
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGPRIVAL_H */
