#ifndef SOLIDSYSLOGPLUSTCPADDRESSPRIVATE_H
#define SOLIDSYSLOGPLUSTCPADDRESSPRIVATE_H

#include "FreeRTOS.h"
#include "FreeRTOS_Sockets.h"

struct SolidSyslogAddress;

struct SolidSyslogPlusTcpAddress
{
    struct freertos_sockaddr Sockaddr;
};

void PlusTcpAddress_Initialise(struct SolidSyslogAddress* base);
void PlusTcpAddress_Cleanup(struct SolidSyslogAddress* base);

static inline struct freertos_sockaddr* SolidSyslogPlusTcpAddress_AsFreertosSockaddr(struct SolidSyslogAddress* base)
{
    return &((struct SolidSyslogPlusTcpAddress*) base)->Sockaddr;
}

static inline const struct freertos_sockaddr* SolidSyslogPlusTcpAddress_AsConstFreertosSockaddr(
    const struct SolidSyslogAddress* base
)
{
    return &((const struct SolidSyslogPlusTcpAddress*) base)->Sockaddr;
}

#endif /* SOLIDSYSLOGPLUSTCPADDRESSPRIVATE_H */
