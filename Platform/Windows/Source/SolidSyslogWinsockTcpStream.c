#include "SolidSyslogWinsockTcpStream.h"
#include "SolidSyslogAddressInternal.h"
#include "SolidSyslogMacros.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogWinsockTcpStreamInternal.h"

#include <mstcpip.h>
#include <stddef.h>

/* File-local forwarders. Taking the address of a __declspec(dllimport)
   Winsock function for static initialisation triggers MSVC C4232 (the address
   isn't a compile-time constant); forwarding through a static function whose
   address IS a compile-time constant avoids the warning without a suppression. */
static SOCKET WSAAPI CallSocket(int af, int type, int protocol);
static int WSAAPI    CallConnect(SOCKET s, const struct sockaddr* name, int namelen);
static int WSAAPI    CallSend(SOCKET s, const char* buf, int len, int flags);
static int WSAAPI    CallRecv(SOCKET s, char* buf, int len, int flags);
static int WSAAPI    CallSetSockOpt(SOCKET s, int level, int optname, const char* optval, int optlen);
static int WSAAPI    CallGetSockOpt(SOCKET s, int level, int optname, char* optval, int* optlen);
static int WSAAPI    CallCloseSocket(SOCKET s);
static int WSAAPI    CallIoctlSocket(SOCKET s, long cmd, u_long* argp);
static int WSAAPI    CallSelect(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const struct timeval* timeout);
static int WSAAPI    CallWSAGetLastError(void);

WinsockTcpStreamSocketFn          WinsockTcpStream_socket          = CallSocket;
WinsockTcpStreamConnectFn         WinsockTcpStream_connect         = CallConnect;
WinsockTcpStreamSendFn            WinsockTcpStream_send            = CallSend;
WinsockTcpStreamRecvFn            WinsockTcpStream_recv            = CallRecv;
WinsockTcpStreamSetSockOptFn      WinsockTcpStream_setsockopt      = CallSetSockOpt;
WinsockTcpStreamGetSockOptFn      WinsockTcpStream_getsockopt      = CallGetSockOpt;
WinsockTcpStreamCloseSocketFn     WinsockTcpStream_closesocket     = CallCloseSocket;
WinsockTcpStreamIoctlSocketFn     WinsockTcpStream_ioctlsocket     = CallIoctlSocket;
WinsockTcpStreamSelectFn          WinsockTcpStream_select          = CallSelect;
WinsockTcpStreamWSAGetLastErrorFn WinsockTcpStream_WSAGetLastError = CallWSAGetLastError;

static SOCKET WSAAPI CallSocket(int af, int type, int protocol)
{
    return socket(af, type, protocol);
}

static int WSAAPI CallConnect(SOCKET s, const struct sockaddr* name, int namelen)
{
    return connect(s, name, namelen);
}

static int WSAAPI CallSend(SOCKET s, const char* buf, int len, int flags)
{
    return send(s, buf, len, flags);
}

static int WSAAPI CallRecv(SOCKET s, char* buf, int len, int flags)
{
    return recv(s, buf, len, flags);
}

static int WSAAPI CallSetSockOpt(SOCKET s, int level, int optname, const char* optval, int optlen)
{
    return setsockopt(s, level, optname, optval, optlen);
}

static int WSAAPI CallGetSockOpt(SOCKET s, int level, int optname, char* optval, int* optlen)
{
    return getsockopt(s, level, optname, optval, optlen);
}

static int WSAAPI CallCloseSocket(SOCKET s)
{
    return closesocket(s);
}

static int WSAAPI CallIoctlSocket(SOCKET s, long cmd, u_long* argp)
{
    return ioctlsocket(s, cmd, argp);
}

static int WSAAPI CallSelect(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const struct timeval* timeout)
{
    /* Winsock select() takes a non-const timeval*; const-cast keeps our seam
       signature const-correct for callers without forcing them to drop const. */
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast) -- C; mirrors the platform signature
    return select(nfds, readfds, writefds, exceptfds, (struct timeval*) timeout);
}

static int WSAAPI CallWSAGetLastError(void)
{
    return WSAGetLastError();
}

enum
{
    /* Caps the time a single connect() attempt can stall the service thread.
       Windows' default connect() to a refused loopback port retries internally
       for ~2 s before returning WSAECONNREFUSED, which throttles the service
       thread's drain rate during an outage and prevents the BlockStore's
       discard policy from firing. Non-blocking connect + select() with this
       timeout keeps each iteration bounded; on outage the next service tick
       retries. 200 ms is comfortable for loopback/LAN and short enough that
       10 failing attempts cost 2 s instead of 20 s. */
    CONNECT_TIMEOUT_MILLISECONDS = 200,
    MILLISECONDS_PER_SECOND      = 1000,
    MICROSECONDS_PER_MILLISECOND = 1000,
    /* Winsock ignores nfds (its fd_set is a literal array, not a bitmask),
       but POSIX-portable callers must pass the highest fd + 1. Pass any
       positive value to keep the call well-formed against either ABI. */
    WINSOCK_NFDS_IGNORED = 1,
    /* Keepalive parameters mirror the POSIX TCP stream so the dead-peer
       detection window is the same on both platforms: idle 45 + 4 * 10 = 85 s
       worst case. Windows has no TCP_USER_TIMEOUT analogue, so the pending-
       write case relies on the OS-default retransmit timeout. */
    KEEPALIVE_IDLE_SECONDS     = 45,
    KEEPALIVE_INTERVAL_SECONDS = 10,
    KEEPALIVE_PROBE_COUNT      = 4
};

struct SolidSyslogWinsockTcpStream
{
    struct SolidSyslogStream base;
    SOCKET                   fd;
};

static bool             Open(struct SolidSyslogStream* self, const struct SolidSyslogAddress* addr);
static bool             Send(struct SolidSyslogStream* self, const void* buffer, size_t size);
static SolidSyslogSsize Read(struct SolidSyslogStream* self, void* buffer, size_t size);
static void             Close(struct SolidSyslogStream* self);

static SOCKET      OpenAndConfigureSocket(void);
static bool        ConfigureSocket(SOCKET fd);
static void        EnableTcpNoDelay(SOCKET fd);
static void        EnableKeepalive(SOCKET fd);
static inline bool IsSocketValid(SOCKET fd);
static bool        ConnectOrCloseOnFailure(struct SolidSyslogWinsockTcpStream* stream, const struct sockaddr_in* sin);
static bool        Connect(SOCKET fd, const struct sockaddr_in* sin);
static bool        SetNonBlocking(SOCKET fd);
static bool        WaitForConnectCompletion(SOCKET fd);
static bool        ReadDeferredConnectError(SOCKET fd);
static bool        WroteAllBytes(int sent, size_t expected);
static inline bool WouldBlock(int wsaError);

SOLIDSYSLOG_STATIC_ASSERT(sizeof(struct SolidSyslogWinsockTcpStream) <= SOLIDSYSLOG_WINSOCK_TCP_STREAM_SIZE,
                          "SOLIDSYSLOG_WINSOCK_TCP_STREAM_SIZE is too small for struct SolidSyslogWinsockTcpStream");

static const struct SolidSyslogWinsockTcpStream DEFAULT_INSTANCE = {
    {Open, Send, Read, Close},
    INVALID_SOCKET,
};

static const struct SolidSyslogWinsockTcpStream DESTROYED_INSTANCE = {
    {NULL, NULL, NULL, NULL},
    INVALID_SOCKET,
};

struct SolidSyslogStream* SolidSyslogWinsockTcpStream_Create(SolidSyslogWinsockTcpStreamStorage* storage)
{
    struct SolidSyslogWinsockTcpStream* stream = (struct SolidSyslogWinsockTcpStream*) storage;
    *stream                                    = DEFAULT_INSTANCE;
    return &stream->base;
}

void SolidSyslogWinsockTcpStream_Destroy(struct SolidSyslogStream* stream)
{
    struct SolidSyslogWinsockTcpStream* self = (struct SolidSyslogWinsockTcpStream*) stream;
    Close(stream);
    *self = DESTROYED_INSTANCE;
}

static bool Open(struct SolidSyslogStream* self, const struct SolidSyslogAddress* addr)
{
    struct SolidSyslogWinsockTcpStream* stream    = (struct SolidSyslogWinsockTcpStream*) self;
    const struct sockaddr_in*           sin       = SolidSyslogAddress_AsConstSockaddrIn(addr);
    bool                                connected = false;

    stream->fd = OpenAndConfigureSocket();
    if (IsSocketValid(stream->fd))
    {
        connected = ConnectOrCloseOnFailure(stream, sin);
    }
    return connected;
}

/* Non-blocking from the start: connect() returns WSAEWOULDBLOCK and the wait
 * is bounded via select(); Send/Read never block the service thread on a
 * wedged peer or a full kernel send buffer. */
static SOCKET OpenAndConfigureSocket(void)
{
    SOCKET fd = WinsockTcpStream_socket(AF_INET, SOCK_STREAM, 0);
    if (IsSocketValid(fd) && !ConfigureSocket(fd))
    {
        WinsockTcpStream_closesocket(fd);
        fd = INVALID_SOCKET;
    }
    return fd;
}

static bool ConfigureSocket(SOCKET fd)
{
    EnableTcpNoDelay(fd);
    EnableKeepalive(fd);
    return SetNonBlocking(fd);
}

static void EnableTcpNoDelay(SOCKET fd)
{
    int enable = 1;
    WinsockTcpStream_setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*) &enable, (int) sizeof(enable));
}

/* Mirrors the POSIX EnableKeepalive — Windows 10 1709+ exposes TCP_KEEPIDLE /
 * TCP_KEEPINTVL / TCP_KEEPCNT via setsockopt (declared in <mstcpip.h>), so the
 * shape matches the POSIX path one-for-one. No TCP_USER_TIMEOUT analogue. */
static void EnableKeepalive(SOCKET fd)
{
    int enable   = 1;
    int idle     = KEEPALIVE_IDLE_SECONDS;
    int interval = KEEPALIVE_INTERVAL_SECONDS;
    int count    = KEEPALIVE_PROBE_COUNT;

    WinsockTcpStream_setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char*) &enable, (int) sizeof(enable));
    WinsockTcpStream_setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, (const char*) &idle, (int) sizeof(idle));
    WinsockTcpStream_setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, (const char*) &interval, (int) sizeof(interval));
    WinsockTcpStream_setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, (const char*) &count, (int) sizeof(count));
}

static inline bool IsSocketValid(SOCKET fd)
{
    return fd != INVALID_SOCKET;
}

static bool ConnectOrCloseOnFailure(struct SolidSyslogWinsockTcpStream* stream, const struct sockaddr_in* sin)
{
    bool connected = Connect(stream->fd, sin);
    if (!connected)
    {
        WinsockTcpStream_closesocket(stream->fd);
        stream->fd = INVALID_SOCKET;
    }
    return connected;
}

/* Non-blocking connect with bounded wait. Windows' default blocking connect()
 * to a refused loopback port retries internally for ~2 s before returning
 * WSAECONNREFUSED — slow enough that the BlockStore service thread's drain
 * rate during an outage is throttled to ~0.5 records/s, preventing the
 * discard policy from firing in BDD outage scenarios. The non-blocking path
 * bounds each connect attempt to CONNECT_TIMEOUT_MILLISECONDS; the socket
 * stays non-blocking thereafter so Send/Read are also fail-fast. */
static bool Connect(SOCKET fd, const struct sockaddr_in* sin)
{
    bool connected = false;
    int  rc        = WinsockTcpStream_connect(fd, (const struct sockaddr*) sin, (int) sizeof(*sin));

    if (rc != SOCKET_ERROR)
    {
        connected = true;
    }
    else if (WinsockTcpStream_WSAGetLastError() == WSAEWOULDBLOCK)
    {
        connected = WaitForConnectCompletion(fd) && ReadDeferredConnectError(fd);
    }
    return connected;
}

static bool SetNonBlocking(SOCKET fd)
{
    u_long mode = 1;
    return WinsockTcpStream_ioctlsocket(fd, (long) FIONBIO, &mode) != SOCKET_ERROR;
}

static bool WaitForConnectCompletion(SOCKET fd)
{
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(fd, &writeSet);

    fd_set errorSet;
    FD_ZERO(&errorSet);
    FD_SET(fd, &errorSet);

    struct timeval timeout = {.tv_sec  = CONNECT_TIMEOUT_MILLISECONDS / MILLISECONDS_PER_SECOND,
                              .tv_usec = (CONNECT_TIMEOUT_MILLISECONDS % MILLISECONDS_PER_SECOND) * MICROSECONDS_PER_MILLISECOND};

    int rc = WinsockTcpStream_select(WINSOCK_NFDS_IGNORED, NULL, &writeSet, &errorSet, &timeout);

    return (rc > 0) && FD_ISSET(fd, &writeSet) && !FD_ISSET(fd, &errorSet);
}

static bool ReadDeferredConnectError(SOCKET fd)
{
    int err    = 0;
    int errlen = (int) sizeof(err);
    int rc     = WinsockTcpStream_getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*) &err, &errlen);

    return (rc != SOCKET_ERROR) && (err == 0);
}

static bool Send(struct SolidSyslogStream* self, const void* buffer, size_t size)
{
    struct SolidSyslogWinsockTcpStream* stream = (struct SolidSyslogWinsockTcpStream*) self;
    int                                 sent   = WinsockTcpStream_send(stream->fd, (const char*) buffer, (int) size, 0);
    bool                                ok     = WroteAllBytes(sent, size);

    if (!ok)
    {
        Close(self);
    }
    return ok;
}

/* Non-blocking single-call contract: short write or any error means the
 * connection is gone; the caller closes and reconnects on the next attempt. */
static bool WroteAllBytes(int sent, size_t expected)
{
    return (sent >= 0) && ((size_t) sent == expected);
}

static SolidSyslogSsize Read(struct SolidSyslogStream* self, void* buffer, size_t size)
{
    struct SolidSyslogWinsockTcpStream* stream = (struct SolidSyslogWinsockTcpStream*) self;
    int                                 n      = WinsockTcpStream_recv(stream->fd, (char*) buffer, (int) size, 0);
    SolidSyslogSsize                    result = -1;

    if (n > 0)
    {
        result = (SolidSyslogSsize) n;
    }
    else if ((n == SOCKET_ERROR) && WouldBlock(WinsockTcpStream_WSAGetLastError()))
    {
        result = 0;
    }
    else
    {
        Close(self);
    }
    return result;
}

static inline bool WouldBlock(int wsaError)
{
    return wsaError == WSAEWOULDBLOCK;
}

static void Close(struct SolidSyslogStream* self)
{
    struct SolidSyslogWinsockTcpStream* stream = (struct SolidSyslogWinsockTcpStream*) self;
    if (IsSocketValid(stream->fd))
    {
        WinsockTcpStream_closesocket(stream->fd);
        stream->fd = INVALID_SOCKET;
    }
}
