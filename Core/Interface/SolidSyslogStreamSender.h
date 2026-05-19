#ifndef SOLIDSYSLOG_STREAM_SENDER_H
#define SOLIDSYSLOG_STREAM_SENDER_H

#include "SolidSyslogEndpoint.h"
#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogSender;

    struct SolidSyslogStreamSenderConfig
    {
        struct SolidSyslogResolver* Resolver;
        struct SolidSyslogStream* Stream;
        SolidSyslogEndpointFunction Endpoint; /* fills host/port; called only on (re)connect */
        SolidSyslogEndpointVersionFunction EndpointVersion; /* polled cheaply on every Send for stale check */
    };

    struct SolidSyslogSender* SolidSyslogStreamSender_Create(const struct SolidSyslogStreamSenderConfig* config);
    void SolidSyslogStreamSender_Destroy(struct SolidSyslogSender * base);

EXTERN_C_END

#endif /* SOLIDSYSLOG_STREAM_SENDER_H */
