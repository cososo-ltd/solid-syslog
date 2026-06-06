#ifndef SOLIDSYSLOGMESSAGEFORMATTER_H
#define SOLIDSYSLOGMESSAGEFORMATTER_H

#include "ExternC.h"

#include <stddef.h>

#include "SolidSyslogHeaderFieldFunction.h"
#include "SolidSyslogTimestamp.h"

EXTERN_C_BEGIN

    struct SolidSyslogFormatter;
    struct SolidSyslogMessage;
    struct SolidSyslogStructuredData;

    /* The per-instance inputs the message formatter reads while building an
     * RFC 5424 frame. Owned by struct SolidSyslog as its single copy; the
     * install/reset sites write through it. */
    struct SolidSyslogMessageFormatterContext
    {
        SolidSyslogClockFunction Clock;
        SolidSyslogHeaderFieldFunction GetHostname;
        void* GetHostnameContext;
        SolidSyslogHeaderFieldFunction GetAppName;
        void* GetAppNameContext;
        SolidSyslogHeaderFieldFunction GetProcessId;
        void* GetProcessIdContext;
        struct SolidSyslogStructuredData** Sd;
        size_t SdCount;
    };

    /* Emits a full RFC 5424 SYSLOG-MSG into f:
     * <PRIVAL>1 TIMESTAMP HOSTNAME APP-NAME PROCID MSGID SD [SP BOM MSG].
     * Captures the timestamp via context->Clock and delegates its formatting
     * to SolidSyslogTimestampFormatter_Format. The SD area emits the per-instance
     * SDs (context->Sd) followed by the per-message SDs (messageSd[0..messageSdCount)),
     * NILVALUE only when neither produces an element. */
    void SolidSyslogMessageFormatter_Format(
        struct SolidSyslogFormatter * formatter,
        const struct SolidSyslogMessage* message,
        const struct SolidSyslogMessageFormatterContext* context,
        struct SolidSyslogStructuredData** messageSd,
        size_t messageSdCount
    );

EXTERN_C_END

#endif /* SOLIDSYSLOGMESSAGEFORMATTER_H */
