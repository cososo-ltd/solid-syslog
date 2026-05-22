#include "SolidSyslogWinsockDatagram.h"

#include <stdbool.h>
#include <stddef.h>
#include <ws2tcpip.h>

#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogUdpPayload.h"
#include "SolidSyslogWinsockAddressPrivate.h"
#include "SolidSyslogWinsockDatagramInternal.h"
#include "SolidSyslogWinsockDatagramPrivate.h"

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

static bool WinsockDatagram_Open(struct SolidSyslogDatagram* base);
static enum SolidSyslogDatagramSendResult WinsockDatagram_SendTo(
    struct SolidSyslogDatagram* base,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
);
static size_t WinsockDatagram_MaxPayload(struct SolidSyslogDatagram* base);
static void WinsockDatagram_Close(struct SolidSyslogDatagram* base);

static inline struct SolidSyslogWinsockDatagram* WinsockDatagram_SelfFromBase(struct SolidSyslogDatagram* base);
static inline bool WinsockDatagram_ConnectIfNeeded(
    struct SolidSyslogWinsockDatagram* self,
    const struct SolidSyslogAddress* addr
);
static inline bool WinsockDatagram_IsSocketValid(SOCKET fd);

void WinsockDatagram_Initialise(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogWinsockDatagram* self = WinsockDatagram_SelfFromBase(base);
    self->Base.Open = WinsockDatagram_Open;
    self->Base.SendTo = WinsockDatagram_SendTo;
    self->Base.MaxPayload = WinsockDatagram_MaxPayload;
    self->Base.Close = WinsockDatagram_Close;
    self->Fd = INVALID_SOCKET;
    self->Connected = false;
}

static inline struct SolidSyslogWinsockDatagram* WinsockDatagram_SelfFromBase(struct SolidSyslogDatagram* base)
{
    return (struct SolidSyslogWinsockDatagram*) base;
}

void WinsockDatagram_Cleanup(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogWinsockDatagram* self = WinsockDatagram_SelfFromBase(base);
    if (WinsockDatagram_IsSocketValid(self->Fd))
    {
        Winsock_closesocket(self->Fd);
        self->Fd = INVALID_SOCKET;
        self->Connected = false;
    }
    /* Overwrite the abstract base with the shared NullDatagram vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullDatagram_Get();
}

static inline bool WinsockDatagram_IsSocketValid(SOCKET fd)
{
    return fd != INVALID_SOCKET;
}

static bool WinsockDatagram_Open(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogWinsockDatagram* self = WinsockDatagram_SelfFromBase(base);
    self->Fd = Winsock_socket(AF_INET, SOCK_DGRAM, 0);
    self->Connected = false;
    return WinsockDatagram_IsSocketValid(self->Fd);
}

static enum SolidSyslogDatagramSendResult WinsockDatagram_SendTo(
    struct SolidSyslogDatagram* base,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
)
{
    struct SolidSyslogWinsockDatagram* self = WinsockDatagram_SelfFromBase(base);
    enum SolidSyslogDatagramSendResult result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED;
    if (WinsockDatagram_ConnectIfNeeded(self, addr))
    {
        const struct sockaddr_in* sin = SolidSyslogWinsockAddress_AsConstSockaddrIn(addr);
        int sent = Winsock_sendto(
            self->Fd,
            (const char*) buffer,
            (int) size,
            0,
            (const struct sockaddr*) sin,
            (int) sizeof(*sin)
        );
        if (sent != SOCKET_ERROR)
        {
            result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT;
        }
        else if (WSAGetLastError() == WSAEMSGSIZE)
        {
            result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE;
        }
        else
        {
            /* generic send failure — result stays Failed */
        }
    }
    return result;
}

static inline bool WinsockDatagram_ConnectIfNeeded(
    struct SolidSyslogWinsockDatagram* self,
    const struct SolidSyslogAddress* addr
)
{
    if (!self->Connected)
    {
        const struct sockaddr_in* sin = SolidSyslogWinsockAddress_AsConstSockaddrIn(addr);
        if (Winsock_connect(self->Fd, (const struct sockaddr*) sin, (int) sizeof(*sin)) != SOCKET_ERROR)
        {
            const int pmtu = IP_PMTUDISC_DO;
            (void) Winsock_setsockopt(self->Fd, IPPROTO_IP, IP_MTU_DISCOVER, (const char*) &pmtu, (int) sizeof(pmtu));
            self->Connected = true;
        }
    }
    return self->Connected;
}

static size_t WinsockDatagram_MaxPayload(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogWinsockDatagram* self = WinsockDatagram_SelfFromBase(base);
    size_t result = SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD;
    if (self->Connected)
    {
        int mtu = 0;
        int optlen = (int) sizeof(mtu);
        if ((Winsock_getsockopt(self->Fd, IPPROTO_IP, IP_MTU, (char*) &mtu, &optlen) != SOCKET_ERROR) && (mtu > 0))
        {
            result = SolidSyslogUdpPayload_FromMtu((size_t) mtu, false);
        }
    }
    return result;
}

static void WinsockDatagram_Close(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogWinsockDatagram* self = WinsockDatagram_SelfFromBase(base);
    if (WinsockDatagram_IsSocketValid(self->Fd))
    {
        Winsock_closesocket(self->Fd);
        self->Fd = INVALID_SOCKET;
        self->Connected = false;
    }
}
