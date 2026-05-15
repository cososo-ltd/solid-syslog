#ifndef SOLIDSYSLOG_H
#define SOLIDSYSLOG_H

#include "ExternC.h"
#include "SolidSyslogPrival.h"

EXTERN_C_BEGIN

    struct SolidSyslogMessage
    {
        enum SolidSyslogFacility Facility;
        enum SolidSyslogSeverity Severity;
        const char* MessageId;
        const char* Msg;
    };

    void SolidSyslog_Log(const struct SolidSyslogMessage* message);
    void SolidSyslog_Service(void);

EXTERN_C_END

#endif /* SOLIDSYSLOG_H */
