#ifndef SOLIDSYSLOGPRIVAL_H
#define SOLIDSYSLOGPRIVAL_H

#include "ExternC.h"

EXTERN_C_BEGIN

    enum SolidSyslogFacility
    {
        SolidSyslogFacility_Kern = 0,
        SolidSyslogFacility_User = 1,
        SolidSyslogFacility_Mail = 2,
        SolidSyslogFacility_Daemon = 3,
        SolidSyslogFacility_Auth = 4,
        SolidSyslogFacility_Syslog = 5,
        SolidSyslogFacility_Lpr = 6,
        SolidSyslogFacility_News = 7,
        SolidSyslogFacility_Uucp = 8,
        SolidSyslogFacility_Cron = 9,
        SolidSyslogFacility_AuthPriv = 10,
        SolidSyslogFacility_Ftp = 11,
        SolidSyslogFacility_Ntp = 12,
        SolidSyslogFacility_Audit = 13,
        SolidSyslogFacility_Alert = 14,
        SolidSyslogFacility_Clock = 15,
        SolidSyslogFacility_Local0 = 16,
        SolidSyslogFacility_Local1 = 17,
        SolidSyslogFacility_Local2 = 18,
        SolidSyslogFacility_Local3 = 19,
        SolidSyslogFacility_Local4 = 20,
        SolidSyslogFacility_Local5 = 21,
        SolidSyslogFacility_Local6 = 22,
        SolidSyslogFacility_Local7 = 23
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
