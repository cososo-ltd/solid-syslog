#ifndef SOLIDSYSLOGWINSOCKADDRESSPRIVATE_H
#define SOLIDSYSLOGWINSOCKADDRESSPRIVATE_H

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogWinsockAddressErrors.h"

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

static inline void WinsockAddress_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogWinsockAddressErrors code
)
{
    SolidSyslog_Error(severity, &WinsockAddressErrorSource, category, code);
}

#endif /* SOLIDSYSLOGWINSOCKADDRESSPRIVATE_H */
