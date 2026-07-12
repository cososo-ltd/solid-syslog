#ifndef SOLIDSYSLOGUDPPAYLOAD_H
#define SOLIDSYSLOGUDPPAYLOAD_H

#include "ExternC.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

EXTERN_C_BEGIN

    /** Largest UDP payload guaranteed to fit an unfragmented IPv6 datagram:
     *  IPv6 minimum MTU 1280 − 40-byte IPv6 header − 8-byte UDP header
     *  (RFC 8200 §5). Used as the last-resort MaxPayload when the OS cannot
     *  report a path MTU. */
    enum
    {
        SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD = 1232
    };

    /** Usable UDP payload for an @p mtu, subtracting the IP header (40 for IPv6,
     *  20 for IPv4) and the 8-byte UDP header. */
    size_t SolidSyslogUdpPayload_FromMtu(size_t mtu, bool isIpv6);

    /* Returns the largest length' <= length such that buffer[0..length' - 1]
     * ends on a UTF-8 codepoint boundary. Walks back over any partial
     * multi-byte sequence at the cut point. Assumes the bytes preceding the
     * cut form valid UTF-8 (the formatter guarantees this — S12.10). */
    size_t SolidSyslogUdpPayload_TrimToCodepointBoundary(const uint8_t* buffer, size_t length);

EXTERN_C_END

#endif /* SOLIDSYSLOGUDPPAYLOAD_H */
