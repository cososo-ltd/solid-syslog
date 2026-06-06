#ifndef SOLIDSYSLOGCONFIG_H
#define SOLIDSYSLOGCONFIG_H

#include <stddef.h>

#include "SolidSyslogHeaderFieldFunction.h"
#include "SolidSyslogTimestamp.h"
#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslog;
    struct SolidSyslogBuffer;
    struct SolidSyslogHeaderField;
    struct SolidSyslogSender;
    struct SolidSyslogStore;
    struct SolidSyslogStructuredData;

    struct SolidSyslogConfig
    {
        struct SolidSyslogBuffer* Buffer;
        struct SolidSyslogSender* Sender;
        SolidSyslogClockFunction Clock;
        SolidSyslogHeaderFieldFunction GetHostname;
        void* GetHostnameContext;
        SolidSyslogHeaderFieldFunction GetAppName;
        void* GetAppNameContext;
        SolidSyslogHeaderFieldFunction GetProcessId;
        void* GetProcessIdContext;
        struct SolidSyslogStore* Store;
        struct SolidSyslogStructuredData** Sd;
        size_t SdCount;
    };

    struct SolidSyslog* SolidSyslog_Create(const struct SolidSyslogConfig* config);
    void SolidSyslog_Destroy(struct SolidSyslog * handle);

EXTERN_C_END

#endif /* SOLIDSYSLOGCONFIG_H */
