#ifndef SOLIDSYSLOGUDPSENDER_H
#define SOLIDSYSLOGUDPSENDER_H

#include "SolidSyslogEndpoint.h"
#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogUdpSenderConfig
    {
        struct SolidSyslogResolver* Resolver;
        struct SolidSyslogDatagram* Datagram;
        SolidSyslogEndpointFunction Endpoint; /* fills host/port; called only on (re)connect */
        SolidSyslogEndpointVersionFunction EndpointVersion; /* polled cheaply on every Send for stale check */
    };

    struct SolidSyslogSender* SolidSyslogUdpSender_Create(const struct SolidSyslogUdpSenderConfig* config);
    void SolidSyslogUdpSender_Destroy(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGUDPSENDER_H */
