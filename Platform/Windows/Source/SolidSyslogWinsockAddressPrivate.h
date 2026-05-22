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
    // cppcheck-suppress cstyleCast -- opaque-to-impl downcast: pool slot is a real SolidSyslogWinsockAddress
    return &((struct SolidSyslogWinsockAddress*) base)->Sockaddr;
}

static inline const struct sockaddr_in* SolidSyslogWinsockAddress_AsConstSockaddrIn(
    const struct SolidSyslogAddress* base
)
{
    // cppcheck-suppress cstyleCast -- opaque-to-impl downcast: pool slot is a real SolidSyslogWinsockAddress
    return &((const struct SolidSyslogWinsockAddress*) base)->Sockaddr;
}

#endif /* SOLIDSYSLOGWINSOCKADDRESSPRIVATE_H */
