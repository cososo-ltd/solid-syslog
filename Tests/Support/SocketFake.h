#ifndef SOCKETFAKE_H
#define SOCKETFAKE_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "ExternC.h"

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
    /* When set, connect returns -1 with errno == errnoValue (e.g. EINPROGRESS so
       the non-blocking-connect path can be exercised). One-shot. */
    void SocketFake_SetConnectFailsWithErrno(int errnoValue);

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
    /* Returns the int optval recorded for the most recent setsockopt call
       matching (level, optname). Captures only int-sized options (optlen ==
       sizeof(int)); other shapes are ignored. Returns 0 if no match. */
    int SocketFake_LastSetSockOptValue(int level, int optname);

    /* getsockopt configuration (models IPPROTO_IP / IP_MTU and SOL_SOCKET / SO_ERROR) */
    void SocketFake_SetIpMtu(int mtu);
    void SocketFake_SetIpMtuLookupFails(bool fails);
    /* SOL_SOCKET / SO_ERROR — read by the non-blocking-connect completion path.
       Defaults to 0 (success) until set. */
    void SocketFake_SetSoError(int err);
    void SocketFake_SetSoErrorLookupFails(bool fails);

    /* getsockopt accessors */
    int SocketFake_GetSockOptCallCount(void);
    int SocketFake_LastGetSockOptLevel(void);
    int SocketFake_LastGetSockOptOptname(void);

    /* fcntl configuration */
    void SocketFake_SetFcntlSetFlFails(bool fails);
    void SocketFake_SetFcntlGetFlReturn(int flags);

    /* fcntl accessors */
    int  SocketFake_FcntlCallCount(void);
    int  SocketFake_LastFcntlCmd(void);
    int  SocketFake_LastFcntlSetFlags(void);
    bool SocketFake_FcntlSetFlSetNonBlocking(void);

    /* select configuration — three independent simulations:
       (1) successful non-blocking-connect completion: SetSelectWritable(true)
           plus SetSoError(0) — fd is writable, SO_ERROR is clear.
       (2) deferred connect failure (typical "connection refused" path on
           POSIX): SetSelectWritable(true) plus SetSoError(ECONNREFUSED) —
           the fd appears writable, getsockopt(SO_ERROR) reveals the error.
       (3) select() reporting fd in exceptfds: SetSelectError(true) — the
           production path additionally rejects fds in the exception set
           via FD_ISSET on errorSet, even though typical kernels surface
           connect failures via simulation (2) rather than exceptfds.
       SetSelectReturn overrides the syscall return value (1 ready, 0 timeout,
       -1 syscall failure). Default: returns 1 (one fd ready, no error). */
    void SocketFake_SetSelectWritable(bool ready);
    void SocketFake_SetSelectError(bool hasError);
    void SocketFake_SetSelectReturn(int value);

    /* select accessors */
    int  SocketFake_SelectCallCount(void);
    long SocketFake_LastSelectTimeoutSec(void);
    long SocketFake_LastSelectTimeoutUsec(void);

    /* close accessors */
    int SocketFake_CloseCallCount(void);
    int SocketFake_LastClosedFd(void);

    /* recv configuration */
    void SocketFake_SetRecvReturn(ssize_t value);
    void SocketFake_FailNextRecvWithErrno(int errnoValue);

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
