#ifndef SOLIDSYSLOGFREERTOSADDRESSPRIVATE_H
#define SOLIDSYSLOGFREERTOSADDRESSPRIVATE_H

#include "FreeRTOS.h"
#include "FreeRTOS_Sockets.h"

struct SolidSyslogAddress;

struct SolidSyslogFreeRtosAddress
{
    struct freertos_sockaddr Sockaddr;
};

void FreeRtosAddress_Initialise(struct SolidSyslogAddress* base);
void FreeRtosAddress_Cleanup(struct SolidSyslogAddress* base);

static inline struct freertos_sockaddr* SolidSyslogFreeRtosAddress_AsFreertosSockaddr(struct SolidSyslogAddress* base)
{
    // cppcheck-suppress cstyleCast -- opaque-to-impl downcast: pool slot is a real SolidSyslogFreeRtosAddress
    return &((struct SolidSyslogFreeRtosAddress*) base)->Sockaddr;
}

static inline const struct freertos_sockaddr* SolidSyslogFreeRtosAddress_AsConstFreertosSockaddr(
    const struct SolidSyslogAddress* base
)
{
    // cppcheck-suppress cstyleCast -- opaque-to-impl downcast: pool slot is a real SolidSyslogFreeRtosAddress
    return &((const struct SolidSyslogFreeRtosAddress*) base)->Sockaddr;
}

#endif /* SOLIDSYSLOGFREERTOSADDRESSPRIVATE_H */
