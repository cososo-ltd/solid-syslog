/** @file
 *  A blocking DNS resolver over Winsock getaddrinfo.
 *
 *  Resolve looks up the endpoint host as an IPv4 address (AF_INET) through a
 *  synchronous getaddrinfo call and writes it into the destination
 *  SolidSyslogAddress; the requested transport selects the socktype hint. A
 *  failed lookup returns false, so the caller's unresolved-host error path runs.
 *
 *  The caller must invoke WSAStartup before use and WSACleanup on shutdown; the
 *  library does not manage the Winsock lifecycle. */
#ifndef SOLIDSYSLOGWINSOCKRESOLVERH
#define SOLIDSYSLOGWINSOCKRESOLVERH

#include "SolidSyslogResolver.h"

EXTERN_C_BEGIN

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullResolver. */
    struct SolidSyslogResolver* SolidSyslogWinsockResolver_Create(void);
    /** Release the pool slot. */
    void SolidSyslogWinsockResolver_Destroy(struct SolidSyslogResolver * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKRESOLVERH */
