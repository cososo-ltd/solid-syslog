#ifndef SOLIDSYSLOG_STREAM_SENDER_H
#define SOLIDSYSLOG_STREAM_SENDER_H

#include <stdint.h>

#include "SolidSyslogEndpoint.h"
#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogSender;

    struct SolidSyslogStreamSenderConfig
    {
        struct SolidSyslogResolver*        resolver;
        struct SolidSyslogStream*          stream;
        SolidSyslogEndpointFunction        endpoint;        /* fills host/port; called only on (re)connect */
        SolidSyslogEndpointVersionFunction endpointVersion; /* polled cheaply on every Send for stale check */
    };

    enum
    {
        SOLIDSYSLOG_STREAM_SENDER_SIZE = (sizeof(intptr_t) * 7) + sizeof(uint32_t)
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_STREAM_SENDER_SIZE + sizeof(intptr_t) - 1) / sizeof(intptr_t)];
    } SolidSyslogStreamSenderStorage;

    struct SolidSyslogSender* SolidSyslogStreamSender_Create(SolidSyslogStreamSenderStorage * storage, const struct SolidSyslogStreamSenderConfig* config);
    void                      SolidSyslogStreamSender_Destroy(struct SolidSyslogSender * sender);

EXTERN_C_END

#endif /* SOLIDSYSLOG_STREAM_SENDER_H */
