#ifndef SOLIDSYSLOGWINSOCKADDRESSPRIVATE_H
#define SOLIDSYSLOGWINSOCKADDRESSPRIVATE_H

#include <stdint.h>
#include <winsock2.h>

struct SolidSyslogAddress;

struct SolidSyslogWinsockAddress
{
    struct sockaddr_in Sockaddr;
};

void WinsockAddress_Initialise(struct SolidSyslogAddress* base);
void WinsockAddress_Cleanup(struct SolidSyslogAddress* base);

static inline struct sockaddr_in* SolidSyslogWinsockAddress_AsSockaddrIn(struct SolidSyslogAddress* base)
{
    return &((struct SolidSyslogWinsockAddress*) base)->Sockaddr;
}

static inline const struct sockaddr_in* SolidSyslogWinsockAddress_AsConstSockaddrIn(
    const struct SolidSyslogAddress* base
)
{
    return &((const struct SolidSyslogWinsockAddress*) base)->Sockaddr;
}

#endif /* SOLIDSYSLOGWINSOCKADDRESSPRIVATE_H */
