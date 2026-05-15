#ifndef SOLIDSYSLOGCONFIG_H
#define SOLIDSYSLOGCONFIG_H

#include <stddef.h>

#include "SolidSyslogStringFunction.h"
#include "SolidSyslogTimestamp.h"
#include "ExternC.h"

EXTERN_C_BEGIN

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

    void SolidSyslog_Create(const struct SolidSyslogConfig* config);
    void SolidSyslog_Destroy(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGCONFIG_H */
