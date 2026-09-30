/* A test stand-in for the VxWorks 6.4 sys/socket.h.
 *
 * VxWorks takes its socket types from a header with the standard name, so this
 * stand-in shadows the host's for the VxWorks test executables only: the host's
 * sendto has a different prototype from VxWorks', and the two cannot both be
 * declared. The generic address is BSD-shaped, with a length byte ahead of the
 * family, as the VxWorks stack has it.
 */
#ifndef VXWORKS64FAKES_SYS_SOCKET_H
#define VXWORKS64FAKES_SYS_SOCKET_H

#ifdef __cplusplus
extern "C"
{
#endif

/* NOLINTBEGIN(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */
#define AF_INET 2
#define SOCK_DGRAM 2
    /* NOLINTEND(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */

    struct sockaddr
    {
        unsigned char sa_len;
        unsigned char sa_family;
        char sa_data[14];
    };

#ifdef __cplusplus
}
#endif

#endif /* VXWORKS64FAKES_SYS_SOCKET_H */
