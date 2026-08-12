#ifndef LWIPDNSFAKE_H
#define LWIPDNSFAKE_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>
#include <stdint.h>

#include "lwip/ip_addr.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void LwipDnsFake_Reset(void);

    /* Immediate err returned by dns_gethostbyname itself. Default ERR_OK
     * (synchronous hit: numeric literal / DNS cache / local hostlist). Set to
     * ERR_INPROGRESS to drive the async path (the wrapper then spins until the
     * stored callback is fired via LwipDnsFake_FireCallback), or ERR_ARG to
     * drive the bad-argument rejection. */
    void LwipDnsFake_SetResult(int8_t err);

    /* The address dns_gethostbyname writes into its out-param on a synchronous
     * ERR_OK return. Defaults to 0.0.0.0. */
    void LwipDnsFake_SetResolvedIp(const ip_addr_t* ip);

    /* dns_gethostbyname spy. */
    unsigned LwipDnsFake_GetHostByNameCallCount(void);
    const char* LwipDnsFake_LastHostname(void);
    ip_addr_t* LwipDnsFake_LastAddrOut(void);

    /* True between an ERR_INPROGRESS dns_gethostbyname and the matching
     * FireCallback - i.e. a dns_found_callback is stored and not yet fired. */
    bool LwipDnsFake_HasPendingCallback(void);

    /* Fires the stored dns_found_callback (the async completion). Pass a
     * non-NULL ipaddr to deliver a resolved address, or NULL to deliver a
     * lookup failure - mirroring lwIP's contract. The host stand-in for the
     * tcpip thread firing the callback while the caller spins; tests drive it
     * from their injected Sleep. */
    void LwipDnsFake_FireCallback(const ip_addr_t* ipaddr);

SOLIDSYSLOG_EXTERN_C_END

#endif /* LWIPDNSFAKE_H */
