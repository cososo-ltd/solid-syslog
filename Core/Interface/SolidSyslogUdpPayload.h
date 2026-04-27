#ifndef SOLIDSYSLOGUDPPAYLOAD_H
#define SOLIDSYSLOGUDPPAYLOAD_H

#include "ExternC.h"

#include <stdbool.h>
#include <stddef.h>

EXTERN_C_BEGIN

    /* IPv6 minimum MTU (1280) − IPv6 header (40) − UDP header (8). RFC 8200 §5.
     * Last-resort fallback when the OS cannot report path MTU. */
    enum
    {
        SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD = 1232
    };

    size_t SolidSyslogUdpPayload_FromMtu(size_t mtu, bool isIpv6);

EXTERN_C_END

#endif /* SOLIDSYSLOGUDPPAYLOAD_H */
