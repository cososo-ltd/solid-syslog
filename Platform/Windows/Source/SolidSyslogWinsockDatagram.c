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
static SOCKET WSAAPI CallSocket(int af, int type, int protocol);
static int WSAAPI CallSendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen);
static int WSAAPI CallCloseSocket(SOCKET s);
static int WSAAPI CallConnect(SOCKET s, const struct sockaddr* name, int namelen);
static int WSAAPI CallSetSockOpt(SOCKET s, int level, int optname, const char* optval, int optlen);
static int WSAAPI CallGetSockOpt(SOCKET s, int level, int optname, char* optval, int* optlen);

WinsockSocketFn Winsock_socket = CallSocket;
WinsockSendToFn Winsock_sendto = CallSendTo;
WinsockCloseSocketFn Winsock_closesocket = CallCloseSocket;
WinsockConnectFn Winsock_connect = CallConnect;
WinsockSetSockOptFn Winsock_setsockopt = CallSetSockOpt;
WinsockGetSockOptFn Winsock_getsockopt = CallGetSockOpt;

static SOCKET WSAAPI CallSocket(int af, int type, int protocol)
{
    return socket(af, type, protocol);
}

static int WSAAPI CallSendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
{
    return sendto(s, buf, len, flags, to, tolen);
}

static int WSAAPI CallCloseSocket(SOCKET s)
{
    return closesocket(s);
}

static int WSAAPI CallConnect(SOCKET s, const struct sockaddr* name, int namelen)
{
    return connect(s, name, namelen);
}

static int WSAAPI CallSetSockOpt(SOCKET s, int level, int optname, const char* optval, int optlen)
{
    return setsockopt(s, level, optname, optval, optlen);
}

static int WSAAPI CallGetSockOpt(SOCKET s, int level, int optname, char* optval, int* optlen)
{
    return getsockopt(s, level, optname, optval, optlen);
}

struct SolidSyslogWinsockDatagram
{
    struct SolidSyslogDatagram base;
    SOCKET fd;
    bool connected;
};

static bool Open(struct SolidSyslogDatagram* self);
static enum SolidSyslogDatagramSendResult SendTo(
    struct SolidSyslogDatagram* self,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
);
static size_t MaxPayload(struct SolidSyslogDatagram* self);
static void Close(struct SolidSyslogDatagram* self);
static inline bool ConnectIfNeeded(struct SolidSyslogWinsockDatagram* datagram, const struct SolidSyslogAddress* addr);
static inline bool IsSocketValid(SOCKET fd);

static struct SolidSyslogWinsockDatagram instance = {.fd = INVALID_SOCKET};

struct SolidSyslogDatagram* SolidSyslogWinsockDatagram_Create(void)
{
    instance.base.Open = Open;
    instance.base.SendTo = SendTo;
    instance.base.MaxPayload = MaxPayload;
    instance.base.Close = Close;
    return &instance.base;
}

void SolidSyslogWinsockDatagram_Destroy(void)
{
    instance.base.Open = NULL;
    instance.base.SendTo = NULL;
    instance.base.MaxPayload = NULL;
    instance.base.Close = NULL;
}

static bool Open(struct SolidSyslogDatagram* self)
{
    struct SolidSyslogWinsockDatagram* datagram = (struct SolidSyslogWinsockDatagram*) self;
    datagram->fd = Winsock_socket(AF_INET, SOCK_DGRAM, 0);
    datagram->connected = false;
    return IsSocketValid(datagram->fd);
}

static inline bool IsSocketValid(SOCKET fd)
{
    return fd != INVALID_SOCKET;
}

static enum SolidSyslogDatagramSendResult SendTo(
    struct SolidSyslogDatagram* self,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
)
{
    struct SolidSyslogWinsockDatagram* datagram = (struct SolidSyslogWinsockDatagram*) self;
    enum SolidSyslogDatagramSendResult result = SolidSyslogDatagramSendResult_Failed;
    if (ConnectIfNeeded(datagram, addr))
    {
        const struct sockaddr_in* sin = SolidSyslogAddress_AsConstSockaddrIn(addr);
        int sent = Winsock_sendto(
            datagram->fd,
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
    }
    return result;
}

static inline bool ConnectIfNeeded(struct SolidSyslogWinsockDatagram* datagram, const struct SolidSyslogAddress* addr)
{
    if (!datagram->connected)
    {
        const struct sockaddr_in* sin = SolidSyslogAddress_AsConstSockaddrIn(addr);
        if (Winsock_connect(datagram->fd, (const struct sockaddr*) sin, (int) sizeof(*sin)) != SOCKET_ERROR)
        {
            const int pmtu = IP_PMTUDISC_DO;
            (void
            ) Winsock_setsockopt(datagram->fd, IPPROTO_IP, IP_MTU_DISCOVER, (const char*) &pmtu, (int) sizeof(pmtu));
            datagram->connected = true;
        }
    }
    return datagram->connected;
}

static size_t MaxPayload(struct SolidSyslogDatagram* self)
{
    struct SolidSyslogWinsockDatagram* datagram = (struct SolidSyslogWinsockDatagram*) self;
    size_t result = SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD;
    if (datagram->connected)
    {
        int mtu = 0;
        int optlen = (int) sizeof(mtu);
        if ((Winsock_getsockopt(datagram->fd, IPPROTO_IP, IP_MTU, (char*) &mtu, &optlen) != SOCKET_ERROR) && (mtu > 0))
        {
            result = SolidSyslogUdpPayload_FromMtu((size_t) mtu, false);
        }
    }
    return result;
}

static void Close(struct SolidSyslogDatagram* self)
{
    struct SolidSyslogWinsockDatagram* datagram = (struct SolidSyslogWinsockDatagram*) self;
    if (IsSocketValid(datagram->fd))
    {
        Winsock_closesocket(datagram->fd);
        datagram->fd = INVALID_SOCKET;
        datagram->connected = false;
    }
}
