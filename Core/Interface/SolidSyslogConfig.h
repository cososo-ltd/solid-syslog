#ifndef SOLIDSYSLOGCONFIG_H
#define SOLIDSYSLOGCONFIG_H

#include <stddef.h>

#include "SolidSyslogStringFunction.h"
#include "SolidSyslogTimestamp.h"
#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslog;
    struct SolidSyslogBuffer;
    struct SolidSyslogFormatter;
    struct SolidSyslogSender;
    struct SolidSyslogStore;
    struct SolidSyslogStructuredData;

    struct SolidSyslogConfig
    {
        struct SolidSyslogBuffer* Buffer;
        struct SolidSyslogSender* Sender;
        SolidSyslogClockFunction Clock;
        SolidSyslogStringFunction GetHostname;
        SolidSyslogStringFunction GetAppName;
        SolidSyslogStringFunction GetProcessId;
        struct SolidSyslogStore* Store;
        struct SolidSyslogStructuredData** Sd;
        size_t SdCount;
    };

    struct SolidSyslog* SolidSyslog_Create(const struct SolidSyslogConfig* config);
    void SolidSyslog_Destroy(struct SolidSyslog * handle);

EXTERN_C_END

#endif /* SOLIDSYSLOGCONFIG_H */
