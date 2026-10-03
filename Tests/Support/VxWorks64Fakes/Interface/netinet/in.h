/* A test stand-in for the VxWorks 6.4 netinet/in.h.
 *
 * Shadows the host's header for the VxWorks test executables only, for the
 * reason given in sys/socket.h beside it. The IPv4 address is BSD-shaped, with
 * a length byte ahead of the family, as the VxWorks stack has it.
 */
#ifndef VXWORKS64FAKES_NETINET_IN_H
#define VXWORKS64FAKES_NETINET_IN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    struct in_addr
    {
        uint32_t s_addr;
    };

    struct sockaddr_in
    {
        unsigned char sin_len;
        unsigned char sin_family;
        uint16_t sin_port;
        struct in_addr sin_addr;
        char sin_zero[8];
    };

    /* Byte-order conversion for a little-endian host, where the fakes run. The
     * real header chooses by the target's byte order. */
    static inline uint16_t htons(uint16_t value)
    {
        return (uint16_t) ((uint16_t) (value << 8U) | (uint16_t) (value >> 8U));
    }

    static inline uint32_t htonl(uint32_t value)
    {
        return ((value & 0x000000FFU) << 24U) | ((value & 0x0000FF00U) << 8U) | ((value & 0x00FF0000U) >> 8U) |
               ((value & 0xFF000000U) >> 24U);
    }

#ifdef __cplusplus
}
#endif

#endif /* VXWORKS64FAKES_NETINET_IN_H */
