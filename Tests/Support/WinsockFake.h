#ifndef WINSOCKFAKE_H
#define WINSOCKFAKE_H

#include "ExternC.h"
#include <stdbool.h>
#include <stddef.h>
#include <winsock2.h>
#include <ws2tcpip.h>

EXTERN_C_BEGIN

    void WinsockFake_Reset(void);

    /* socket configuration */
    void WinsockFake_SetSocketFails(bool fails);

    /* socket accessors */
    int    WinsockFake_SocketCallCount(void);
    SOCKET WinsockFake_SocketFd(void);
    int    WinsockFake_SocketDomain(void);
    int    WinsockFake_SocketType(void);

    /* sendto configuration */
    void WinsockFake_SetSendtoFails(bool fails);
    void WinsockFake_FailNextSendtoWithLastError(int wsaError);

    /* sendto accessors */
    int         WinsockFake_SendtoCallCount(void);
    const char* WinsockFake_LastBufAsString(void);
    size_t      WinsockFake_LastLen(void);
    int         WinsockFake_LastFlags(void);
    int         WinsockFake_LastAddrFamily(void);
    const char* WinsockFake_LastAddrAsString(void);
    int         WinsockFake_LastPort(void);
    int         WinsockFake_LastAddrLen(void);
    SOCKET      WinsockFake_LastSendtoFd(void);

    /* connect configuration */
    void WinsockFake_SetConnectFails(bool fails);

    /* connect accessors */
    int         WinsockFake_ConnectCallCount(void);
    SOCKET      WinsockFake_LastConnectFd(void);
    int         WinsockFake_LastConnectPort(void);
    const char* WinsockFake_LastConnectAddrAsString(void);

    /* send configuration */
    void WinsockFake_SetSendFails(bool fails);
    void WinsockFake_SetSendReturn(int value);

    /* send accessors */
    int         WinsockFake_SendCallCount(void);
    const char* WinsockFake_SendBufAsString(int callIndex);
    size_t      WinsockFake_SendLen(int callIndex);
    int         WinsockFake_SendFlags(int callIndex);
    SOCKET      WinsockFake_LastSendFd(void);

    /* recv configuration */
    void WinsockFake_SetRecvReturn(int value);

    /* recv accessors */
    int         WinsockFake_RecvCallCount(void);
    SOCKET      WinsockFake_LastRecvFd(void);
    const void* WinsockFake_LastRecvBuf(void);
    size_t      WinsockFake_LastRecvLen(void);
    int         WinsockFake_LastRecvFlags(void);

    /* setsockopt accessors */
    int  WinsockFake_SetSockOptCallCount(void);
    int  WinsockFake_LastSetSockOptLevel(void);
    int  WinsockFake_LastSetSockOptOptname(void);
    bool WinsockFake_HasSetSockOpt(int level, int optname);

    /* getsockopt configuration (currently models IPPROTO_IP / IP_MTU only) */
    void WinsockFake_SetIpMtu(int mtu);
    void WinsockFake_SetIpMtuLookupFails(bool fails);

    /* getsockopt accessors */
    int WinsockFake_GetSockOptCallCount(void);

    /* closesocket accessors */
    int    WinsockFake_CloseCallCount(void);
    SOCKET WinsockFake_LastClosedFd(void);

    /* getaddrinfo configuration */
    void WinsockFake_SetGetAddrInfoFails(bool fails);

    /* getaddrinfo accessors */
    int         WinsockFake_GetAddrInfoCallCount(void);
    const char* WinsockFake_LastGetAddrInfoHostname(void);
    int         WinsockFake_LastGetAddrInfoSocktype(void);

    /* freeaddrinfo accessors */
    int WinsockFake_FreeAddrInfoCallCount(void);

    /* Fake Winsock functions — injected into production via UT_PTR_SET. */
    SOCKET WSAAPI WinsockFake_socket(int af, int type, int protocol);
    int WSAAPI    WinsockFake_sendto(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen);
    int WSAAPI    WinsockFake_connect(SOCKET s, const struct sockaddr* name, int namelen);
    int WSAAPI    WinsockFake_send(SOCKET s, const char* buf, int len, int flags);
    int WSAAPI    WinsockFake_recv(SOCKET s, char* buf, int len, int flags);
    int WSAAPI    WinsockFake_setsockopt(SOCKET s, int level, int optname, const char* optval, int optlen);
    int WSAAPI    WinsockFake_getsockopt(SOCKET s, int level, int optname, char* optval, int* optlen);
    int WSAAPI    WinsockFake_closesocket(SOCKET s);
    int WSAAPI    WinsockFake_getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res);
    void WSAAPI   WinsockFake_freeaddrinfo(struct addrinfo * res);

EXTERN_C_END

#endif /* WINSOCKFAKE_H */
