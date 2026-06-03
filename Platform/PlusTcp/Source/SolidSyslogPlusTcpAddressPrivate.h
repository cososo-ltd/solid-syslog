#ifndef SOLIDSYSLOGPLUSTCPADDRESSPRIVATE_H
#define SOLIDSYSLOGPLUSTCPADDRESSPRIVATE_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "FreeRTOS_Sockets.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPlusTcpAddressErrors.h"
#include "SolidSyslogPrival.h"

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

static inline void PlusTcpAddress_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogPlusTcpAddressErrors code
)
{
    SolidSyslog_Error(severity, &PlusTcpAddressErrorSource, category, code);
}

#endif /* SOLIDSYSLOGPLUSTCPADDRESSPRIVATE_H */
