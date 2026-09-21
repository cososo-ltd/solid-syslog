#ifndef LWIPNETDBFAKE_H
#define LWIPNETDBFAKE_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

#include "lwip/netdb.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void LwipNetdbFake_Reset(void);

    /* The IPv4 address a successful lookup answers with, as a dotted literal.
     * Defaults to 0.0.0.0. */
    void LwipNetdbFake_SetIpv4Result(const char* literal);

    /* The family stamped on the answer, in both ai_family and the sockaddr.
     * Defaults to AF_INET. Set it to AF_INET6 to hand back a result the hint
     * did not ask for, which the real stack narrows but the adapter must not
     * rely on it doing. */
    void LwipNetdbFake_SetResultFamily(int family);

    /* The value lwip_getaddrinfo returns. Default 0, a successful lookup; a
     * non-zero value is returned with no result, as upstream does. */
    void LwipNetdbFake_SetReturn(int value);

    /* lwip_getaddrinfo spy. */
    unsigned LwipNetdbFake_GetAddrInfoCallCount(void);
    const char* LwipNetdbFake_LastNodename(void);
    const char* LwipNetdbFake_LastServname(void);
    int LwipNetdbFake_LastHintsFamily(void);

    /* lwip_freeaddrinfo spy. The count is what proves the adapter gave the
     * MEMP_NETDB entry back, on every path that obtained one. */
    unsigned LwipNetdbFake_FreeAddrInfoCallCount(void);
    const struct addrinfo* LwipNetdbFake_LastFreed(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* LWIPNETDBFAKE_H */
