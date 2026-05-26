#ifndef SOLIDSYSLOGPOSIXADDRESSPRIVATE_H
#define SOLIDSYSLOGPOSIXADDRESSPRIVATE_H

#include <netinet/in.h>
#include <stdint.h>

struct SolidSyslogAddress;

struct SolidSyslogPosixAddress
{
    struct sockaddr_in Sockaddr;
};

void PosixAddress_Initialise(struct SolidSyslogAddress* base);
void PosixAddress_Cleanup(struct SolidSyslogAddress* base);

static inline struct sockaddr_in* SolidSyslogPosixAddress_AsSockaddrIn(struct SolidSyslogAddress* base)
{
    return &((struct SolidSyslogPosixAddress*) base)->Sockaddr;
}

static inline const struct sockaddr_in* SolidSyslogPosixAddress_AsConstSockaddrIn(const struct SolidSyslogAddress* base)
{
    return &((const struct SolidSyslogPosixAddress*) base)->Sockaddr;
}

#endif /* SOLIDSYSLOGPOSIXADDRESSPRIVATE_H */
