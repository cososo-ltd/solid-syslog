/* A test stand-in for the VxWorks 6.4 netinet/tcp.h.
 *
 * Shadows the host's header for the VxWorks test executables only, for the
 * reason given in sys/socket.h. Supplies the one TCP-level option
 * Platform/VxWorks64 sets, with the value the VxWorks header gives it.
 */
#ifndef VXWORKS64FAKES_NETINET_TCP_H
#define VXWORKS64FAKES_NETINET_TCP_H

/* NOLINTBEGIN(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */
#define TCP_NODELAY 0x01
/* NOLINTEND(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */

#endif /* VXWORKS64FAKES_NETINET_TCP_H */
