#ifndef SOLIDSYSLOG_STREAM_SENDER_H
#define SOLIDSYSLOG_STREAM_SENDER_H

#include <stdint.h>

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

    enum
    {
        SOLIDSYSLOG_STREAM_SENDER_SIZE = (sizeof(intptr_t) * 7U) + sizeof(uint32_t)
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_STREAM_SENDER_SIZE + sizeof(intptr_t) - 1U) / sizeof(intptr_t)];
    } SolidSyslogStreamSenderStorage;

    struct SolidSyslogSender* SolidSyslogStreamSender_Create(
        SolidSyslogStreamSenderStorage * storage,
        const struct SolidSyslogStreamSenderConfig* config
    );
    void SolidSyslogStreamSender_Destroy(struct SolidSyslogSender * base);

EXTERN_C_END

#endif /* SOLIDSYSLOG_STREAM_SENDER_H */
