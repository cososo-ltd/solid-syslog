#include "SolidSyslogPosixTcpStream.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "SolidSyslogNullStream.h"
#include "SolidSyslogPosixAddressPrivate.h"
#include "SolidSyslogPosixTcpStreamPrivate.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"

struct SolidSyslogAddress;

enum
{
    INVALID_FD = -1,
    /* Caps the time a single connect() attempt can stall the service thread.
       Mirrors the Winsock value: 200 ms is comfortable for loopback/LAN and
       short enough that 10 failing attempts cost 2 s instead of 20+ s. */
    CONNECT_TIMEOUT_MICROSECONDS = 200000,
    /* Keepalive parameters — bound the dead-peer detection window when the
       socket is idle. Worst case: 45 + 4 * 10 = 85 s before ETIMEDOUT.
       TCP_USER_TIMEOUT covers the pending-write case (where keepalive does
       not fire) by capping how long unacked data can sit in the send queue. */
    KEEPALIVE_IDLE_SECONDS = 45,
    KEEPALIVE_INTERVAL_SECONDS = 10,
    KEEPALIVE_PROBE_COUNT = 4,
    USER_TIMEOUT_MILLISECONDS = 30000
};

static bool PosixTcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);
static bool PosixTcpStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size);
static SolidSyslogSsize PosixTcpStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size);
static void PosixTcpStream_Close(struct SolidSyslogStream* base);

static inline struct SolidSyslogPosixTcpStream* PosixTcpStream_SelfFromBase(struct SolidSyslogStream* base);

static int PosixTcpStream_OpenAndConfigureSocket(void);
static bool PosixTcpStream_ConfigureSocket(int fd);
static void PosixTcpStream_EnableTcpNoDelay(int fd);
static void PosixTcpStream_EnableKeepalive(int fd);
static bool PosixTcpStream_SetNonBlocking(int fd);
static inline bool PosixTcpStream_IsFileDescriptorValid(int fd);
static bool PosixTcpStream_ConnectOrCloseOnFailure(
    struct SolidSyslogPosixTcpStream* self,
    const struct sockaddr_in* sin
);
static bool PosixTcpStream_Connect(int fd, const struct sockaddr_in* sin);
static bool PosixTcpStream_WaitForConnectCompletion(int fd);
static bool PosixTcpStream_ReadDeferredConnectError(int fd);
static bool PosixTcpStream_WroteAllBytes(ssize_t sent, size_t expected);
static inline bool PosixTcpStream_WouldBlock(void);

void PosixTcpStream_Initialise(struct SolidSyslogStream* base)
{
    struct SolidSyslogPosixTcpStream* self = PosixTcpStream_SelfFromBase(base);
    self->Base.Open = PosixTcpStream_Open;
    self->Base.Send = PosixTcpStream_Send;
    self->Base.Read = PosixTcpStream_Read;
    self->Base.Close = PosixTcpStream_Close;
    self->Fd = INVALID_FD;
}

static inline struct SolidSyslogPosixTcpStream* PosixTcpStream_SelfFromBase(struct SolidSyslogStream* base)
{
    return (struct SolidSyslogPosixTcpStream*) base;
}

void PosixTcpStream_Cleanup(struct SolidSyslogStream* base)
{
    struct SolidSyslogPosixTcpStream* self = PosixTcpStream_SelfFromBase(base);
    if (PosixTcpStream_IsFileDescriptorValid(self->Fd))
    {
        close(self->Fd);
        self->Fd = INVALID_FD;
    }
    /* Overwrite the abstract base with the shared NullStream vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullStream_Get();
}

static inline bool PosixTcpStream_IsFileDescriptorValid(int fd)
{
    return (fd >= 0);
}

static bool PosixTcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr)
{
    struct SolidSyslogPosixTcpStream* self = PosixTcpStream_SelfFromBase(base);
    const struct sockaddr_in* sin = SolidSyslogPosixAddress_AsConstSockaddrIn(addr);
    bool connected = false;

    self->Fd = PosixTcpStream_OpenAndConfigureSocket();
    if (PosixTcpStream_IsFileDescriptorValid(self->Fd))
    {
        connected = PosixTcpStream_ConnectOrCloseOnFailure(self, sin);
    }
    return connected;
}

/* Non-blocking from the start: connect() reports EINPROGRESS, the wait is
 * bounded by select(), and PosixTcpStream_Send/PosixTcpStream_Read never block the service thread on a
 * wedged peer or a full kernel send buffer. */
static int PosixTcpStream_OpenAndConfigureSocket(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (PosixTcpStream_IsFileDescriptorValid(fd) && !PosixTcpStream_ConfigureSocket(fd))
    {
        close(fd);
        fd = INVALID_FD;
    }
    return fd;
}

static bool PosixTcpStream_ConfigureSocket(int fd)
{
    PosixTcpStream_EnableTcpNoDelay(fd);
    PosixTcpStream_EnableKeepalive(fd);
    return PosixTcpStream_SetNonBlocking(fd);
}

static void PosixTcpStream_EnableTcpNoDelay(int fd)
{
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
}

/* Enable kernel TCP keepalive so a dead peer is surfaced as ETIMEDOUT during
 * idle periods, not on the next PosixTcpStream_Send. TCP_USER_TIMEOUT covers the orthogonal
 * pending-write case (keepalive only fires on a fully idle socket). Linux is
 * the POSIX target — TCP_KEEP* and TCP_USER_TIMEOUT are all available there;
 * other POSIX targets are out of scope until we actually port to one. */
static void PosixTcpStream_EnableKeepalive(int fd)
{
    int enable = 1;
    int idle = KEEPALIVE_IDLE_SECONDS;
    int interval = KEEPALIVE_INTERVAL_SECONDS;
    int count = KEEPALIVE_PROBE_COUNT;
    int userTimeout = USER_TIMEOUT_MILLISECONDS;

    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
    setsockopt(fd, IPPROTO_TCP, TCP_USER_TIMEOUT, &userTimeout, sizeof(userTimeout));
}

static bool PosixTcpStream_SetNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    return (flags >= 0) && (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
}

static bool PosixTcpStream_ConnectOrCloseOnFailure(
    struct SolidSyslogPosixTcpStream* self,
    const struct sockaddr_in* sin
)
{
    bool connected = PosixTcpStream_Connect(self->Fd, sin);
    if (!connected)
    {
        close(self->Fd);
        self->Fd = INVALID_FD;
    }
    return connected;
}

/* Non-blocking connect with bounded wait. connect() returns immediately:
 *   0           — connected (loopback success path).
 *   -1 EINPROGRESS — connect started; wait via select() up to
 *                    CONNECT_TIMEOUT_MICROSECONDS, then read SO_ERROR to
 *                    distinguish completed-success from deferred-failure.
 *   -1 other    — immediate fail-fast (refused, unreachable, etc.). */
static bool PosixTcpStream_Connect(int fd, const struct sockaddr_in* sin)
{
    bool connected = false;
    int rc = connect(fd, (const struct sockaddr*) sin, sizeof(*sin));

    if (rc == 0)
    {
        connected = true;
    }
    else if (errno == EINPROGRESS)
    {
        connected = PosixTcpStream_WaitForConnectCompletion(fd) && PosixTcpStream_ReadDeferredConnectError(fd);
    }
    else
    {
        /* immediate fail-fast (refused, unreachable, etc.) — connected stays false */
    }
    return connected;
}

static bool PosixTcpStream_WaitForConnectCompletion(int fd)
{
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(fd, &writeSet);

    fd_set errorSet;
    FD_ZERO(&errorSet);
    FD_SET(fd, &errorSet);

    struct timeval timeout = {.tv_sec = 0, .tv_usec = CONNECT_TIMEOUT_MICROSECONDS};

    int rc = select(fd + 1, NULL, &writeSet, &errorSet, &timeout);
    return (rc > 0) && FD_ISSET(fd, &writeSet) && !FD_ISSET(fd, &errorSet);
}

static bool PosixTcpStream_ReadDeferredConnectError(int fd)
{
    int err = 0;
    socklen_t errlen = (socklen_t) sizeof(err);
    int rc = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
    return (rc == 0) && (err == 0);
}

static bool PosixTcpStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size)
{
    struct SolidSyslogPosixTcpStream* self = PosixTcpStream_SelfFromBase(base);
    ssize_t sent = send(self->Fd, buffer, size, MSG_NOSIGNAL);
    bool ok = PosixTcpStream_WroteAllBytes(sent, size);

    if (!ok)
    {
        PosixTcpStream_Close(base);
    }
    return ok;
}

/* Non-blocking single-call contract: short write or any error means the
 * connection is gone; the caller closes and reconnects on the next attempt. */
static bool PosixTcpStream_WroteAllBytes(ssize_t sent, size_t expected)
{
    return (sent >= 0) && ((size_t) sent == expected);
}

static SolidSyslogSsize PosixTcpStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size)
{
    struct SolidSyslogPosixTcpStream* self = PosixTcpStream_SelfFromBase(base);
    ssize_t n = recv(self->Fd, buffer, size, 0);
    SolidSyslogSsize result = -1;

    if (n > 0)
    {
        result = (SolidSyslogSsize) n;
    }
    else if ((n < 0) && PosixTcpStream_WouldBlock())
    {
        result = 0;
    }
    else
    {
        PosixTcpStream_Close(base);
    }
    return result;
}

static inline bool PosixTcpStream_WouldBlock(void)
{
    return (errno == EAGAIN) || (errno == EWOULDBLOCK);
}

static void PosixTcpStream_Close(struct SolidSyslogStream* base)
{
    struct SolidSyslogPosixTcpStream* self = PosixTcpStream_SelfFromBase(base);
    if (PosixTcpStream_IsFileDescriptorValid(self->Fd))
    {
        close(self->Fd);
        self->Fd = INVALID_FD;
    }
}
