/** @file
 *  A DNS resolver over FreeRTOS-Plus-TCP's FreeRTOS_getaddrinfo.
 *
 *  Resolve looks up the endpoint host as an IPv4 address (AF_INET4) through
 *  FreeRTOS_getaddrinfo and writes it, with the per-call port, into the
 *  destination SolidSyslogAddress; the requested transport selects the socktype
 *  hint (stream for TCP, datagram otherwise). A failed lookup returns false, so
 *  the caller's unresolved-host error path runs. */
#ifndef SOLIDSYSLOGPLUSTCPRESOLVER_H
#define SOLIDSYSLOGPLUSTCPRESOLVER_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogResolver;

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullResolver. */
    struct SolidSyslogResolver* SolidSyslogPlusTcpResolver_Create(void);
    /** Release the pool slot. */
    void SolidSyslogPlusTcpResolver_Destroy(struct SolidSyslogResolver * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSTCPRESOLVER_H */
