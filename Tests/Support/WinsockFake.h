#ifndef WINSOCKFAKE_H
#define WINSOCKFAKE_H

#include "ExternC.h"
#include <stdbool.h>
#include <stddef.h>
#include <winsock2.h>
#include <ws2tcpip.h>

EXTERN_C_BEGIN

    void WinsockFake_Reset(void);

    void WinsockFake_SetSocketFails(bool fails);

    int WinsockFake_SocketCallCount(void);
    SOCKET WinsockFake_SocketFd(void);
    int WinsockFake_SocketDomain(void);
    int WinsockFake_SocketType(void);

    void WinsockFake_SetSendtoFails(bool fails);
    void WinsockFake_FailNextSendtoWithLastError(int wsaError);

    int WinsockFake_SendtoCallCount(void);
    const char* WinsockFake_LastBufAsString(void);
    size_t WinsockFake_LastLen(void);
    int WinsockFake_LastFlags(void);
    int WinsockFake_LastAddrFamily(void);
    const char* WinsockFake_LastAddrAsString(void);
    int WinsockFake_LastPort(void);
    int WinsockFake_LastAddrLen(void);
    SOCKET WinsockFake_LastSendtoFd(void);

    void WinsockFake_SetConnectFails(bool fails);
    /* When set, connect returns SOCKET_ERROR with WSAGetLastError == wsaError
       (e.g. WSAEWOULDBLOCK so the non-blocking-connect path can be exercised). */
    void WinsockFake_SetConnectFailsWithLastError(int wsaError);

    int WinsockFake_ConnectCallCount(void);
    SOCKET WinsockFake_LastConnectFd(void);
    int WinsockFake_LastConnectPort(void);
    const char* WinsockFake_LastConnectAddrAsString(void);

    void WinsockFake_SetSendFails(bool fails);
    void WinsockFake_SetSendReturn(int value);

    int WinsockFake_SendCallCount(void);
    const char* WinsockFake_SendBufAsString(int callIndex);
    size_t WinsockFake_SendLen(int callIndex);
    int WinsockFake_SendFlags(int callIndex);
    SOCKET WinsockFake_LastSendFd(void);

    void WinsockFake_SetRecvReturn(int value);
    /* When set, the next recv returns SOCKET_ERROR with WSAGetLastError == wsaError
       (e.g. WSAEWOULDBLOCK for the non-blocking would-block path). One-shot. */
    void WinsockFake_FailNextRecvWithLastError(int wsaError);

    int WinsockFake_RecvCallCount(void);
    SOCKET WinsockFake_LastRecvFd(void);
    const void* WinsockFake_LastRecvBuf(void);
    size_t WinsockFake_LastRecvLen(void);
    int WinsockFake_LastRecvFlags(void);

    int WinsockFake_SetSockOptCallCount(void);
    int WinsockFake_LastSetSockOptLevel(void);
    int WinsockFake_LastSetSockOptOptname(void);
    bool WinsockFake_HasSetSockOpt(int level, int optname);
    /* Returns the int optval recorded for the most recent setsockopt call
       matching (level, optname). Captures only int-sized options (optlen ==
       sizeof(int)); other shapes are ignored. Returns 0 if no match. */
    int WinsockFake_LastSetSockOptValue(int level, int optname);

    void WinsockFake_SetIpMtu(int mtu);
    void WinsockFake_SetIpMtuLookupFails(bool fails);
    /* SOL_SOCKET / SO_ERROR — read by the non-blocking-connect completion
       path. Defaults to 0 (success) until set. */
    void WinsockFake_SetSoError(int err);
    void WinsockFake_SetSoErrorLookupFails(bool fails);

    int WinsockFake_GetSockOptCallCount(void);
    int WinsockFake_LastGetSockOptLevel(void);
    int WinsockFake_LastGetSockOptOptname(void);

    int WinsockFake_CloseCallCount(void);
    SOCKET WinsockFake_LastClosedFd(void);

    void WinsockFake_SetGetAddrInfoFails(bool fails);

    int WinsockFake_GetAddrInfoCallCount(void);
    const char* WinsockFake_LastGetAddrInfoHostname(void);
    int WinsockFake_LastGetAddrInfoSocktype(void);

    int WinsockFake_FreeAddrInfoCallCount(void);

    void WinsockFake_SetIoctlSocketFails(bool fails);

    int WinsockFake_IoctlSocketCallCount(void);
    SOCKET WinsockFake_LastIoctlSocketFd(void);
    long WinsockFake_LastIoctlSocketCmd(void);
    /* Last argp value written into ioctlsocket — for FIONBIO this is the
       non-blocking flag (1 = non-blocking, 0 = blocking). */
    u_long WinsockFake_LastIoctlSocketArg(void);
    /* All recorded ioctlsocket FIONBIO arg values, in call order, so tests
       can assert the producer flips non-blocking on then off again around
       connect. */
    int WinsockFake_FionbioCallCount(void);
    u_long WinsockFake_FionbioArgAt(int callIndex);

    /* select configuration. ready=true makes select() report fd writable in
       writefds; ready=false leaves writefds empty (timeout). hasError=true
       additionally sets fd in exceptfds. returnValue overrides the int return:
       1 for ready, 0 for timeout, SOCKET_ERROR for error. By default select
       returns 1 (one fd ready: writable, no error). */
    void WinsockFake_SetSelectWritable(bool ready);
    void WinsockFake_SetSelectError(bool hasError);
    void WinsockFake_SetSelectReturn(int value);

    int WinsockFake_SelectCallCount(void);
    long WinsockFake_LastSelectTimeoutSec(void);
    long WinsockFake_LastSelectTimeoutUsec(void);

    /* WSAGetLastError shim — defaults to whatever WSASetLastError set. */
    int WSAAPI WinsockFake_WSAGetLastError(void);

    /* Fake Winsock functions — injected into production via UT_PTR_SET. */
    SOCKET WSAAPI WinsockFake_socket(int af, int type, int protocol);
    int WSAAPI WinsockFake_sendto(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen);
    int WSAAPI WinsockFake_connect(SOCKET s, const struct sockaddr* name, int namelen);
    int WSAAPI WinsockFake_send(SOCKET s, const char* buf, int len, int flags);
    int WSAAPI WinsockFake_recv(SOCKET s, char* buf, int len, int flags);
    int WSAAPI WinsockFake_setsockopt(SOCKET s, int level, int optname, const char* optval, int optlen);
    int WSAAPI WinsockFake_getsockopt(SOCKET s, int level, int optname, char* optval, int* optlen);
    int WSAAPI WinsockFake_closesocket(SOCKET s);
    int WSAAPI WinsockFake_ioctlsocket(SOCKET s, long cmd, u_long* argp);
    int WSAAPI
    WinsockFake_select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const struct timeval* timeout);
    int WSAAPI
    WinsockFake_getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res);
    void WSAAPI WinsockFake_freeaddrinfo(struct addrinfo * res);

EXTERN_C_END

#endif /* WINSOCKFAKE_H */
