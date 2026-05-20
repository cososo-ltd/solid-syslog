#ifndef SOLIDSYSLOG_H
#define SOLIDSYSLOG_H

#include "ExternC.h"
#include "SolidSyslogPrival.h"

EXTERN_C_BEGIN

    struct SolidSyslog;

    struct SolidSyslogMessage
    {
        enum SolidSyslogFacility Facility;
        enum SolidSyslogSeverity Severity;
        const char* MessageId;
        const char* Msg;
    };

    void SolidSyslog_Log(struct SolidSyslog * handle, const struct SolidSyslogMessage* message);
    void SolidSyslog_Service(struct SolidSyslog * handle);

EXTERN_C_END

#endif /* SOLIDSYSLOG_H */
