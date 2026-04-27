#include "SolidSyslogWinsockDatagram.h"
#include "SolidSyslogAddressInternal.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogUdpPayload.h"
#include "SolidSyslogWinsockDatagramInternal.h"

#include <stdbool.h>
#include <stddef.h>

/* File-local forwarders. Taking the address of a __declspec(dllimport)
   Winsock function for static initialisation triggers MSVC C4232 (the address
   isn't a compile-time constant); forwarding through a static function whose
   address IS a compile-time constant avoids the warning without a suppression. */
static SOCKET WSAAPI CallSocket(int af, int type, int protocol);
static int WSAAPI    CallSendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen);
static int WSAAPI    CallCloseSocket(SOCKET s);

WinsockSocketFn      Winsock_socket      = CallSocket;
WinsockSendToFn      Winsock_sendto      = CallSendTo;
WinsockCloseSocketFn Winsock_closesocket = CallCloseSocket;

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

static bool                               Open(struct SolidSyslogDatagram* self);
static enum SolidSyslogDatagramSendResult SendTo(struct SolidSyslogDatagram* self, const void* buffer, size_t size, const struct SolidSyslogAddress* addr);
static size_t                             MaxPayload(struct SolidSyslogDatagram* self);
static void                               Close(struct SolidSyslogDatagram* self);
static inline bool                        IsSocketValid(SOCKET fd);

struct SolidSyslogWinsockDatagram
{
    struct SolidSyslogDatagram base;
    SOCKET                     fd;
};

static struct SolidSyslogWinsockDatagram instance = {.fd = INVALID_SOCKET};

struct SolidSyslogDatagram* SolidSyslogWinsockDatagram_Create(void)
{
    instance.base.Open       = Open;
    instance.base.SendTo     = SendTo;
    instance.base.MaxPayload = MaxPayload;
    instance.base.Close      = Close;
    return &instance.base;
}

void SolidSyslogWinsockDatagram_Destroy(void)
{
    instance.base.Open       = NULL;
    instance.base.SendTo     = NULL;
    instance.base.MaxPayload = NULL;
    instance.base.Close      = NULL;
}

static bool Open(struct SolidSyslogDatagram* self)
{
    struct SolidSyslogWinsockDatagram* datagram = (struct SolidSyslogWinsockDatagram*) self;
    datagram->fd                                = Winsock_socket(AF_INET, SOCK_DGRAM, 0);
    return IsSocketValid(datagram->fd);
}

static inline bool IsSocketValid(SOCKET fd)
{
    return fd != INVALID_SOCKET;
}

static enum SolidSyslogDatagramSendResult SendTo(struct SolidSyslogDatagram* self, const void* buffer, size_t size, const struct SolidSyslogAddress* addr)
{
    struct SolidSyslogWinsockDatagram* datagram = (struct SolidSyslogWinsockDatagram*) self;
    const struct sockaddr_in*          sin      = SolidSyslogAddress_AsConstSockaddrIn(addr);
    int sent = Winsock_sendto(datagram->fd, (const char*) buffer, (int) size, 0, (const struct sockaddr*) sin, (int) sizeof(*sin));
    return (sent != SOCKET_ERROR) ? SOLIDSYSLOG_DATAGRAM_SENT : SOLIDSYSLOG_DATAGRAM_FAILED;
}

static size_t MaxPayload(struct SolidSyslogDatagram* self)
{
    (void) self;
    return SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD;
}

static void Close(struct SolidSyslogDatagram* self)
{
    struct SolidSyslogWinsockDatagram* datagram = (struct SolidSyslogWinsockDatagram*) self;
    if (IsSocketValid(datagram->fd))
    {
        Winsock_closesocket(datagram->fd);
        datagram->fd = INVALID_SOCKET;
    }
}
