#include "SolidSyslogWinsockDatagram.h"
#include "SolidSyslogAddressInternal.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogUdpPayload.h"
#include "SolidSyslogWinsockDatagramInternal.h"

#include <stdbool.h>
#include <stddef.h>
#include <ws2tcpip.h>

/* File-local forwarders. Taking the address of a __declspec(dllimport)
   Winsock function for static initialisation triggers MSVC C4232 (the address
   isn't a compile-time constant); forwarding through a static function whose
   address IS a compile-time constant avoids the warning without a suppression. */
static SOCKET WSAAPI WinsockDatagram_CallSocket(int af, int type, int protocol);
static int WSAAPI
WinsockDatagram_CallSendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen);
static int WSAAPI WinsockDatagram_CallCloseSocket(SOCKET s);
static int WSAAPI WinsockDatagram_CallConnect(SOCKET s, const struct sockaddr* name, int namelen);
static int WSAAPI WinsockDatagram_CallSetSockOpt(SOCKET s, int level, int optname, const char* optval, int optlen);
static int WSAAPI WinsockDatagram_CallGetSockOpt(SOCKET s, int level, int optname, char* optval, int* optlen);

WinsockSocketFn Winsock_socket = WinsockDatagram_CallSocket;
WinsockSendToFn Winsock_sendto = WinsockDatagram_CallSendTo;
WinsockCloseSocketFn Winsock_closesocket = WinsockDatagram_CallCloseSocket;
WinsockConnectFn Winsock_connect = WinsockDatagram_CallConnect;
WinsockSetSockOptFn Winsock_setsockopt = WinsockDatagram_CallSetSockOpt;
WinsockGetSockOptFn Winsock_getsockopt = WinsockDatagram_CallGetSockOpt;

static SOCKET WSAAPI WinsockDatagram_CallSocket(int af, int type, int protocol)
{
    return socket(af, type, protocol);
}

static int WSAAPI
WinsockDatagram_CallSendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
{
    return sendto(s, buf, len, flags, to, tolen);
}

static int WSAAPI WinsockDatagram_CallCloseSocket(SOCKET s)
{
    return closesocket(s);
}

static int WSAAPI WinsockDatagram_CallConnect(SOCKET s, const struct sockaddr* name, int namelen)
{
    return connect(s, name, namelen);
}

static int WSAAPI WinsockDatagram_CallSetSockOpt(SOCKET s, int level, int optname, const char* optval, int optlen)
{
    return setsockopt(s, level, optname, optval, optlen);
}

static int WSAAPI WinsockDatagram_CallGetSockOpt(SOCKET s, int level, int optname, char* optval, int* optlen)
{
    return getsockopt(s, level, optname, optval, optlen);
}

struct SolidSyslogWinsockDatagram
{
    struct SolidSyslogDatagram Base;
    SOCKET Fd;
    bool Connected;
};

static bool WinsockDatagram_Open(struct SolidSyslogDatagram* self);
static enum SolidSyslogDatagramSendResult WinsockDatagram_SendTo(
    struct SolidSyslogDatagram* self,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
);
static size_t WinsockDatagram_MaxPayload(struct SolidSyslogDatagram* self);
static void WinsockDatagram_Close(struct SolidSyslogDatagram* self);
static inline bool WinsockDatagram_ConnectIfNeeded(
    struct SolidSyslogWinsockDatagram* datagram,
    const struct SolidSyslogAddress* addr
);
static inline bool WinsockDatagram_IsSocketValid(SOCKET fd);

static struct SolidSyslogWinsockDatagram instance = {.Fd = INVALID_SOCKET};

struct SolidSyslogDatagram* SolidSyslogWinsockDatagram_Create(void)
{
    instance.Base.Open = WinsockDatagram_Open;
    instance.Base.SendTo = WinsockDatagram_SendTo;
    instance.Base.MaxPayload = WinsockDatagram_MaxPayload;
    instance.Base.Close = WinsockDatagram_Close;
    return &instance.Base;
}

void SolidSyslogWinsockDatagram_Destroy(void)
{
    instance.Base.Open = NULL;
    instance.Base.SendTo = NULL;
    instance.Base.MaxPayload = NULL;
    instance.Base.Close = NULL;
}

static bool WinsockDatagram_Open(struct SolidSyslogDatagram* self)
{
    struct SolidSyslogWinsockDatagram* datagram = (struct SolidSyslogWinsockDatagram*) self;
    datagram->Fd = Winsock_socket(AF_INET, SOCK_DGRAM, 0);
    datagram->Connected = false;
    return WinsockDatagram_IsSocketValid(datagram->Fd);
}

static inline bool WinsockDatagram_IsSocketValid(SOCKET fd)
{
    return fd != INVALID_SOCKET;
}

static enum SolidSyslogDatagramSendResult WinsockDatagram_SendTo(
    struct SolidSyslogDatagram* self,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
)
{
    struct SolidSyslogWinsockDatagram* datagram = (struct SolidSyslogWinsockDatagram*) self;
    enum SolidSyslogDatagramSendResult result = SolidSyslogDatagramSendResult_Failed;
    if (WinsockDatagram_ConnectIfNeeded(datagram, addr))
    {
        const struct sockaddr_in* sin = SolidSyslogAddress_AsConstSockaddrIn(addr);
        int sent = Winsock_sendto(
            datagram->Fd,
            (const char*) buffer,
            (int) size,
            0,
            (const struct sockaddr*) sin,
            (int) sizeof(*sin)
        );
        if (sent != SOCKET_ERROR)
        {
            result = SolidSyslogDatagramSendResult_Sent;
        }
        else if (WSAGetLastError() == WSAEMSGSIZE)
        {
            result = SolidSyslogDatagramSendResult_Oversize;
        }
        else
        {
            /* generic send failure — result stays Failed */
        }
    }
    return result;
}

static inline bool WinsockDatagram_ConnectIfNeeded(
    struct SolidSyslogWinsockDatagram* datagram,
    const struct SolidSyslogAddress* addr
)
{
    if (!datagram->Connected)
    {
        const struct sockaddr_in* sin = SolidSyslogAddress_AsConstSockaddrIn(addr);
        if (Winsock_connect(datagram->Fd, (const struct sockaddr*) sin, (int) sizeof(*sin)) != SOCKET_ERROR)
        {
            const int pmtu = IP_PMTUDISC_DO;
            (void
            ) Winsock_setsockopt(datagram->Fd, IPPROTO_IP, IP_MTU_DISCOVER, (const char*) &pmtu, (int) sizeof(pmtu));
            datagram->Connected = true;
        }
    }
    return datagram->Connected;
}

static size_t WinsockDatagram_MaxPayload(struct SolidSyslogDatagram* self)
{
    struct SolidSyslogWinsockDatagram* datagram = (struct SolidSyslogWinsockDatagram*) self;
    size_t result = SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD;
    if (datagram->Connected)
    {
        int mtu = 0;
        int optlen = (int) sizeof(mtu);
        if ((Winsock_getsockopt(datagram->Fd, IPPROTO_IP, IP_MTU, (char*) &mtu, &optlen) != SOCKET_ERROR) && (mtu > 0))
        {
            result = SolidSyslogUdpPayload_FromMtu((size_t) mtu, false);
        }
    }
    return result;
}

static void WinsockDatagram_Close(struct SolidSyslogDatagram* self)
{
    struct SolidSyslogWinsockDatagram* datagram = (struct SolidSyslogWinsockDatagram*) self;
    if (WinsockDatagram_IsSocketValid(datagram->Fd))
    {
        Winsock_closesocket(datagram->Fd);
        datagram->Fd = INVALID_SOCKET;
        datagram->Connected = false;
    }
}
