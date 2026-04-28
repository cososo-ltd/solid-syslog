#ifndef SOCKETFAKE_H
#define SOCKETFAKE_H

#include "ExternC.h"
#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>

EXTERN_C_BEGIN

    void SocketFake_Reset(void);

    /* sendto configuration */
    void SocketFake_SetSendtoFails(bool fails);
    void SocketFake_FailNextSendtoWithErrno(int errnoValue);

    /* sendto accessors */
    int         SocketFake_SendtoCallCount(void);
    const void* SocketFake_LastBuf(void);
    const char* SocketFake_LastBufAsString(void);
    size_t      SocketFake_LastLen(void);
    int         SocketFake_LastFlags(void);
    int         SocketFake_LastAddrFamily(void);
    const char* SocketFake_LastAddrAsString(void);
    int         SocketFake_LastPort(void);
    socklen_t   SocketFake_LastAddrLen(void);
    int         SocketFake_LastSendtoFd(void);

    /* socket configuration */
    void SocketFake_SetSocketFails(bool fails);

    /* socket accessors */
    int SocketFake_SocketCallCount(void);
    int SocketFake_SocketFd(void);
    int SocketFake_SocketDomain(void);
    int SocketFake_SocketType(void);

    /* send configuration */
    void SocketFake_SetSendFails(bool fails);
    void SocketFake_FailSendOnCall(int callNumber);
    void SocketFake_SetSendReturn(ssize_t value);
    void SocketFake_FailNextSendWithErrno(int errnoValue);

    /* send accessors */
    int         SocketFake_SendCallCount(void);
    const char* SocketFake_SendBufAsString(int callIndex);
    size_t      SocketFake_SendLen(int callIndex);
    int         SocketFake_LastSendFd(void);
    int         SocketFake_SendFlags(int callIndex);

    /* connect configuration */
    void SocketFake_SetConnectFails(bool fails);

    /* connect accessors */
    int         SocketFake_ConnectCallCount(void);
    int         SocketFake_LastConnectFd(void);
    int         SocketFake_LastConnectPort(void);
    const char* SocketFake_LastConnectAddrAsString(void);

    /* setsockopt accessors */
    int  SocketFake_SetSockOptCallCount(void);
    int  SocketFake_LastSetSockOptLevel(void);
    int  SocketFake_LastSetSockOptOptname(void);
    bool SocketFake_HasSetSockOpt(int level, int optname);

    /* getsockopt configuration (currently models IPPROTO_IP / IP_MTU only) */
    void SocketFake_SetIpMtu(int mtu);
    void SocketFake_SetIpMtuLookupFails(bool fails);

    /* getsockopt accessors */
    int SocketFake_GetSockOptCallCount(void);

    /* close accessors */
    int SocketFake_CloseCallCount(void);
    int SocketFake_LastClosedFd(void);

    /* recv configuration */
    void SocketFake_SetRecvReturn(ssize_t value);

    /* recv accessors */
    int         SocketFake_RecvCallCount(void);
    int         SocketFake_LastRecvFd(void);
    const void* SocketFake_LastRecvBuf(void);
    size_t      SocketFake_LastRecvLen(void);
    int         SocketFake_LastRecvFlags(void);

    /* getaddrinfo configuration */
    void SocketFake_SetGetAddrInfoFails(bool fails);

    /* getaddrinfo accessors */
    int         SocketFake_GetAddrInfoCallCount(void);
    const char* SocketFake_LastGetAddrInfoHostname(void);
    int         SocketFake_LastGetAddrInfoSocktype(void);

    /* freeaddrinfo accessors */
    int SocketFake_FreeAddrInfoCallCount(void);

EXTERN_C_END

#endif /* SOCKETFAKE_H */
