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

    /** Make connectWithTimeout answer ERROR with errno set to the value given;
     *  zero restores success. */
    void VxWorks64NetFake_FailConnectWithErrno(int errnoValue);

    unsigned VxWorks64NetFake_ConnectWithTimeoutCallCount(void);

    int VxWorks64NetFake_LastConnectFd(void);

    const struct sockaddr* VxWorks64NetFake_LastConnectAddress(void);

    int VxWorks64NetFake_LastConnectAddressLength(void);

    /** Whether the last connectWithTimeout was given an interval at all; without
     *  one it blocks as a plain connect does. */
    bool VxWorks64NetFake_LastConnectWasBounded(void);

    /** The interval the last connectWithTimeout was given, copied when it was
     *  called. */
    long VxWorks64NetFake_LastConnectTimeoutSeconds(void);

    long VxWorks64NetFake_LastConnectTimeoutMicroseconds(void);

    /** Make setsockopt answer ERROR for this level and option; the stack
     *  declines options it does not support. */
    void VxWorks64NetFake_RefuseSocketOption(int level, int optname);

    unsigned VxWorks64NetFake_SetsockoptCallCount(void);

    /** Whether setsockopt was asked, on the descriptor socket hands back, to set
     *  this level and option to this int value. */
    bool VxWorks64NetFake_SocketOptionWasSetTo(int level, int optname, int value);

    unsigned VxWorks64NetFake_CloseCallCount(void);

    int VxWorks64NetFake_LastClosedFd(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* VXWORKS64NETFAKE_H */
