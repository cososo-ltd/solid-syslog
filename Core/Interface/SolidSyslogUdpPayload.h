#ifndef SOLIDSYSLOGUDPPAYLOAD_H
#define SOLIDSYSLOGUDPPAYLOAD_H

#include "ExternC.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

EXTERN_C_BEGIN

    /* IPv6 minimum MTU (1280) − IPv6 header (40) − UDP header (8). RFC 8200 §5.
     * Last-resort fallback when the OS cannot report path MTU. */
    enum
    {
        SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD = 1232
    };

    size_t SolidSyslogUdpPayload_FromMtu(size_t mtu, bool isIpv6);

    /* Returns the largest length' <= length such that buffer[0..length' - 1]
     * ends on a UTF-8 codepoint boundary. Walks back over any partial
     * multi-byte sequence at the cut point. Assumes the bytes preceding the
     * cut form valid UTF-8 (the formatter guarantees this — S12.10). */
    size_t SolidSyslogUdpPayload_TrimToCodepointBoundary(const uint8_t* buffer, size_t length);

EXTERN_C_END

#endif /* SOLIDSYSLOGUDPPAYLOAD_H */
