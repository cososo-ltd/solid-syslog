#include "SolidSyslogUdpPayload.h"

enum
{
    IPV4_HEADER_BYTES = 20,
    IPV6_HEADER_BYTES = 40,
    UDP_HEADER_BYTES  = 8
};

size_t SolidSyslogUdpPayload_FromMtu(size_t mtu, bool isIpv6)
{
    size_t overhead = (isIpv6 ? IPV6_HEADER_BYTES : IPV4_HEADER_BYTES) + UDP_HEADER_BYTES;
    return mtu > overhead ? mtu - overhead : 0;
}
