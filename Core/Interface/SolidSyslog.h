#ifndef SOLIDSYSLOG_H
#define SOLIDSYSLOG_H

#include "ExternC.h"
#include "SolidSyslogPrival.h"

#include <stddef.h>

EXTERN_C_BEGIN

    struct SolidSyslog;
    struct SolidSyslogStructuredData;

    struct SolidSyslogMessage
    {
        enum SolidSyslogFacility Facility;
        enum SolidSyslogSeverity Severity;
        const char* MessageId;
        const char* Msg;
    };

    void SolidSyslog_Log(struct SolidSyslog * handle, const struct SolidSyslogMessage* message);

    /* As SolidSyslog_Log, but attaches sd[0..sdCount) as caller-built SD-ELEMENTs to this one
       message, emitted after the per-instance SDs registered at Create. The SD objects need only
       live for the duration of the call. SolidSyslog_Log is exactly this with sd = NULL, count = 0. */
    void SolidSyslog_LogWithSd(
        struct SolidSyslog * handle,
        const struct SolidSyslogMessage* message,
        struct SolidSyslogStructuredData** sd,
        size_t sdCount
    );

    void SolidSyslog_Service(struct SolidSyslog * handle);

EXTERN_C_END

#endif /* SOLIDSYSLOG_H */
