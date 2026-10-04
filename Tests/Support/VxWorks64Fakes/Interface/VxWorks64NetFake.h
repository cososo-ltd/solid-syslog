#ifndef VXWORKS64NETFAKE_H
#define VXWORKS64NETFAKE_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void VxWorks64NetFake_Reset(void);

    /** The value inet_addr answers; the address in network byte order, or
     *  (unsigned long) ERROR for a string that is not a dotted literal. */
    void VxWorks64NetFake_SetInetAddrReturn(unsigned long value);

    const char* VxWorks64NetFake_LastInetAddrString(void);

    /** The value hostGetByName answers; the address in network byte order, or
     *  ERROR for a name it cannot resolve. */
    void VxWorks64NetFake_SetHostGetByNameReturn(int value);

    unsigned VxWorks64NetFake_HostGetByNameCallCount(void);

    const char* VxWorks64NetFake_LastHostGetByNameName(void);

    /** The name gethostname answers. As the kernel's does, gethostname copies
     *  at most the length it is given, so a name that fills it arrives without
     *  its terminator. sethostname sets the same name. */
    void VxWorks64NetFake_SetHostname(const char* name);

    /** Make gethostname answer ERROR. What it leaves in the buffer is then
     *  unspecified, so the fake leaves a marker there, which a caller that
     *  prints the buffer anyway sends. */
    void VxWorks64NetFake_FailGethostname(void);

    /** The buffer length the last gethostname was given. */
    int VxWorks64NetFake_LastGethostnameLength(void);

    /** Make socket answer ERROR, as the stack does when it cannot make one. */
    void VxWorks64NetFake_SetSocketFails(bool fails);

    unsigned VxWorks64NetFake_SocketCallCount(void);

    int VxWorks64NetFake_LastSocketDomain(void);

    int VxWorks64NetFake_LastSocketType(void);

    int VxWorks64NetFake_LastSocketProtocol(void);

    /** The descriptor socket hands back when it succeeds. */
    int VxWorks64NetFake_SocketFd(void);

    /** Make sendto answer ERROR with errno set to the value given. */
    void VxWorks64NetFake_FailSendtoWithErrno(int errnoValue);

    unsigned VxWorks64NetFake_SendtoCallCount(void);

    int VxWorks64NetFake_LastSendtoFd(void);

    const char* VxWorks64NetFake_LastSendtoBuf(void);

    /** The bytes sendto was last given, copied when it was called and ended as a
     *  string, so they can be read after the caller's buffer has gone. */
    const char* VxWorks64NetFake_LastSendtoPayload(void);

    int VxWorks64NetFake_LastSendtoLen(void);

    int VxWorks64NetFake_LastSendtoFlags(void);

    const struct sockaddr* VxWorks64NetFake_LastSendtoTo(void);

    int VxWorks64NetFake_LastSendtoToLen(void);

    unsigned VxWorks64NetFake_CloseCallCount(void);

    int VxWorks64NetFake_LastClosedFd(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* VXWORKS64NETFAKE_H */
