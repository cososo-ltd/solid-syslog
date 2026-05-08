#ifndef SOLIDSYSLOGADDRESSINTERNAL_H
#define SOLIDSYSLOGADDRESSINTERNAL_H

#include "SolidSyslogAddress.h"
#include "SolidSyslogMacros.h"

#include "FreeRTOS.h"
#include "FreeRTOS_Sockets.h"

SOLIDSYSLOG_STATIC_ASSERT(sizeof(struct freertos_sockaddr) <= SOLIDSYSLOG_ADDRESS_SIZE, SolidSyslogAddressStorage_too_small_for_freertos_sockaddr);

static inline const struct freertos_sockaddr* SolidSyslogAddress_AsConstFreertosSockaddr(const struct SolidSyslogAddress* address)
{
    const uint8_t* bytes = (const uint8_t*) address;
    return (const struct freertos_sockaddr*) bytes;
}

static inline struct freertos_sockaddr* SolidSyslogAddress_AsFreertosSockaddr(struct SolidSyslogAddress* address)
{
    uint8_t* bytes = (uint8_t*) address;
    return (struct freertos_sockaddr*) bytes;
}

#endif /* SOLIDSYSLOGADDRESSINTERNAL_H */
