/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPosixTcpStream.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPosixAddressPrivate.h"
#include "SolidSyslogPosixTcpStreamErrors.h"
#include "SolidSyslogPosixTcpStreamPrivate.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamCategories.h"
#include "SolidSyslogTunables.h"

/* Linux rejects a keepalive option outside these ranges with EINVAL, and the
 * setsockopt results are not checked, so an out-of-range tunable would leave the
 * kernel default in place while TCP_USER_TIMEOUT is derived from the tunable. */
#if SOLIDSYSLOG_TCP_KEEPALIVE_IDLE_SECONDS > 32767
#error "SOLIDSYSLOG_TCP_KEEPALIVE_IDLE_SECONDS must be <= 32767 on Linux"
#endif

#if SOLIDSYSLOG_TCP_KEEPALIVE_INTERVAL_SECONDS > 32767
#error "SOLIDSYSLOG_TCP_KEEPALIVE_INTERVAL_SECONDS must be <= 32767 on Linux"
#endif

#if SOLIDSYSLOG_TCP_KEEPALIVE_PROBE_COUNT > 127
#error "SOLIDSYSLOG_TCP_KEEPALIVE_PROBE_COUNT must be <= 127 on Linux"
#endif

const struct SolidSyslogErrorSource SolidSyslogPosixTcpStreamErrorSource = {
    "PosixTcpStream",
    SolidSyslogTcpStreamErrors_Name
};

struct SolidSyslogAddress;

enum
{
    INVALID_FD = -1,
    MILLISECONDS_PER_SECOND = 1000
};

static uint32_t PosixTcpStream_NullConnectTimeoutGetter(void* context);

static bool PosixTcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);
static bool PosixTcpStream_TakeSocketAndConnect(struct SolidSyslogPosixTcpStream* self, const struct sockaddr_in* sin);
static bool PosixTcpStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size);
static SolidSyslogSsize PosixTcpStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size);
static void PosixTcpStream_Close(struct SolidSyslogStream* base);
static uint32_t PosixTcpStream_Version(struct SolidSyslogStream* base);

static inline struct SolidSyslogPosixTcpStream* PosixTcpStream_SelfFromBase(struct SolidSyslogStream* base);
static inline bool PosixTcpStream_ConfigProvidesGetter(const struct SolidSyslogPosixTcpStreamConfig* config);

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
static bool PosixTcpStream_Connect(int fd, const struct sockaddr_in* sin, long timeoutMicros);
static bool PosixTcpStream_WaitForConnectCompletion(int fd, long timeoutMicros);
static bool PosixTcpStream_ReadDeferredConnectError(int fd);
static bool PosixTcpStream_ReadSocketError(int fd, int* code);
static void PosixTcpStream_ReportConnectFailure(
    enum SolidSyslogSeverity severity,
    enum SolidSyslogTcpStreamErrors detail
);
static void PosixTcpStream_ReportConnectFailureWithErrno(
    enum SolidSyslogSeverity severity,
    enum SolidSyslogTcpStreamErrors detail,
    int nativeErrno
);
static void PosixTcpStream_ReportUnfinishedWait(int fd, int selectResult, int selectErrno);
static inline bool PosixTcpStream_IsRemoteConnectError(int connectErrno);
static inline bool PosixTcpStream_WaitTimedOut(int selectResult);
static long PosixTcpStream_ResolveConnectTimeoutMicros(struct SolidSyslogPosixTcpStream* self);
static bool PosixTcpStream_PeerIsStillThere(struct SolidSyslogPosixTcpStream* self);
static bool PosixTcpStream_WroteAllBytes(ssize_t sent, size_t expected);
static inline bool PosixTcpStream_WouldBlock(int err);

void SolidSyslogPosixTcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogPosixTcpStreamConfig* config
)
{
    static const struct SolidSyslogPosixTcpStream DefaultPosixTcpStream = {
        .Base =
            {.Open = PosixTcpStream_Open,
             .Send = PosixTcpStream_Send,
             .Read = PosixTcpStream_Read,
             .Close = PosixTcpStream_Close,
             .Version = PosixTcpStream_Version},
        .Config = {.GetConnectTimeoutMs = PosixTcpStream_NullConnectTimeoutGetter, .ConnectTimeoutContext = NULL},
        .Fd = INVALID_FD,
    };

    struct SolidSyslogPosixTcpStream* self = PosixTcpStream_SelfFromBase(base);
    *self = DefaultPosixTcpStream;
    if (PosixTcpStream_ConfigProvidesGetter(config) == true)
    {
        self->Config = *config;
    }
}

static inline bool PosixTcpStream_ConfigProvidesGetter(const struct SolidSyslogPosixTcpStreamConfig* config)
{
    return (config != NULL) && (config->GetConnectTimeoutMs != NULL);
}

/* Null Object substituted when the integrator does not install a getter -
 * returns the compile-time tunable so the bounded-wait path has a single
 * code path regardless of whether the integrator wired runtime tuning. */
static uint32_t PosixTcpStream_NullConnectTimeoutGetter(void* context)
{
    (void) context;
    return (uint32_t) SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS;
}

static inline struct SolidSyslogPosixTcpStream* PosixTcpStream_SelfFromBase(struct SolidSyslogStream* base)
{
    return (struct SolidSyslogPosixTcpStream*) base;
}

void SolidSyslogPosixTcpStream_Cleanup(struct SolidSyslogStream* base)
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
    bool connected = false;
    if (PosixTcpStream_IsFileDescriptorValid(self->Fd))
    {
        /* Refused by contract, not reconnected: what is held stays as it is. */
        PosixTcpStream_Report(
            SOLIDSYSLOG_BAD_ARGUMENT_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_ARGUMENT,
            SOLIDSYSLOG_TCP_STREAM_ERROR_ALREADY_OPEN
        );
    }
    else
    {
        connected = PosixTcpStream_TakeSocketAndConnect(self, SolidSyslogPosixAddress_AsConstSockaddrIn(addr));
    }
    return connected;
}

static bool PosixTcpStream_TakeSocketAndConnect(struct SolidSyslogPosixTcpStream* self, const struct sockaddr_in* sin)
{
    bool connected = false;
    self->Fd = PosixTcpStream_OpenAndConfigureSocket();
    if (PosixTcpStream_IsFileDescriptorValid(self->Fd))
    {
        connected = PosixTcpStream_ConnectOrCloseOnFailure(self, sin);
    }
    return connected;
}

/* Every connect failure is one category with the detail naming which step
 * failed, so a portable handler reacts to "no connection" without knowing
 * POSIX. Each failing step reports its own, because each is the only place
 * that knows which one it was, and the only place the errno it left can be
 * read. */
static void PosixTcpStream_ReportConnectFailure(
    enum SolidSyslogSeverity severity,
    enum SolidSyslogTcpStreamErrors detail
)
{
    PosixTcpStream_Report(severity, SOLIDSYSLOG_CAT_STREAM_CONNECT_FAILED, detail);
}

/* The fault, with the errno behind it when the call left one. */
static void PosixTcpStream_ReportConnectFailureWithErrno(
    enum SolidSyslogSeverity severity,
    enum SolidSyslogTcpStreamErrors detail,
    int nativeErrno
)
{
    if (nativeErrno != 0)
    {
        SolidSyslog_ErrorWithNativeCode(
            severity,
            &SolidSyslogPosixTcpStreamErrorSource,
            SOLIDSYSLOG_CAT_STREAM_CONNECT_FAILED,
            (int32_t) detail,
            (int32_t) nativeErrno
        );
    }
    else
    {
        PosixTcpStream_ReportConnectFailure(severity, detail);
    }
}

/* Non-blocking from the start: connect() reports EINPROGRESS, the wait is
 * bounded by select(), and PosixTcpStream_Send/PosixTcpStream_Read never block the service thread on a
 * wedged peer or a full kernel send buffer. */
static int PosixTcpStream_OpenAndConfigureSocket(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    /* Capture errno immediately after socket (MISRA 22.10). */
    int socketErrno = (fd < 0) ? errno : 0;
    if (!PosixTcpStream_IsFileDescriptorValid(fd))
    {
        PosixTcpStream_ReportConnectFailureWithErrno(
            SOLIDSYSLOG_STREAM_CONNECT_LOCAL_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_ENDPOINT_UNAVAILABLE,
            socketErrno
        );
    }
    else if (!PosixTcpStream_ConfigureSocket(fd))
    {
        close(fd);
        fd = INVALID_FD;
    }
    else
    {
        /* configured - ready to connect */
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
 * idle periods, not on the next PosixTcpStream_Send. TCP_USER_TIMEOUT bounds a
 * write the peer never acknowledges, where keepalive does not probe. On Linux a
 * user timeout also ends keepalive, overriding the probe count: the connection
 * drops at the first unanswered probe once the user timeout has elapsed since
 * anything was received. Setting it to the keepalive window, the idle period
 * plus the interval across every probe, keeps the probe count meaningful and
 * bounds an unacknowledged write by the same window. Linux is the POSIX
 * target - TCP_KEEP* and TCP_USER_TIMEOUT are all available there; other POSIX
 * targets are out of scope until we actually port to one. */
static void PosixTcpStream_EnableKeepalive(int fd)
{
    int enable = 1;
    int idle = (int) SOLIDSYSLOG_TCP_KEEPALIVE_IDLE_SECONDS;
    int interval = (int) SOLIDSYSLOG_TCP_KEEPALIVE_INTERVAL_SECONDS;
    int count = (int) SOLIDSYSLOG_TCP_KEEPALIVE_PROBE_COUNT;
    int userTimeout = (idle + (interval * count)) * MILLISECONDS_PER_SECOND;

    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
    setsockopt(fd, IPPROTO_TCP, TCP_USER_TIMEOUT, &userTimeout, sizeof(userTimeout));
}

/* A socket the kernel will not make non-blocking is of no use: the bounded
 * connect, and Send and Read never blocking the service thread, all rest on
 * it. The attempt is given up here, and this is the step that reports it. */
static bool PosixTcpStream_SetNonBlocking(int fd)
{
    bool nonBlocking = false;
    int flags = fcntl(fd, F_GETFL, 0);
    /* Capture errno immediately after each fcntl (MISRA 22.10). */
    int fcntlErrno = (flags < 0) ? errno : 0;
    if (flags >= 0)
    {
        int rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        fcntlErrno = (rc == -1) ? errno : 0;
        nonBlocking = (rc != -1);
    }
    if (!nonBlocking)
    {
        PosixTcpStream_ReportConnectFailureWithErrno(
            SOLIDSYSLOG_STREAM_CONNECT_LOCAL_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_ENDPOINT_UNAVAILABLE,
            fcntlErrno
        );
    }
    return nonBlocking;
}

static bool PosixTcpStream_ConnectOrCloseOnFailure(
    struct SolidSyslogPosixTcpStream* self,
    const struct sockaddr_in* sin
)
{
    long timeoutMicros = PosixTcpStream_ResolveConnectTimeoutMicros(self);
    bool connected = PosixTcpStream_Connect(self->Fd, sin, timeoutMicros);
    if (!connected)
    {
        close(self->Fd);
        self->Fd = INVALID_FD;
    }
    return connected;
}

/* Bridges the integrator-installed getter (or the Null Object substituted in
 * Initialise) to the bounded select() deadline. Invoked on every connect
 * attempt so a runtime-tunable value takes effect on the next reconnect. */
static long PosixTcpStream_ResolveConnectTimeoutMicros(struct SolidSyslogPosixTcpStream* self)
{
    uint32_t ms = self->Config.GetConnectTimeoutMs(self->Config.ConnectTimeoutContext);
    uint32_t boundedMs = (ms > SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS_MAX) ? SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS_MAX : ms;
    return (long) boundedMs * 1000L;
}

/* Non-blocking connect with bounded wait. connect() returns immediately:
 *   0           - connected (loopback success path).
 *   -1 EINPROGRESS - connect started; wait via select() up to
 *                    CONNECT_TIMEOUT_MICROSECONDS, then read SO_ERROR to
 *                    distinguish completed-success from deferred-failure.
 *   -1 other    - immediate fail-fast (refused, unreachable, etc.). */
static bool PosixTcpStream_Connect(int fd, const struct sockaddr_in* sin, long timeoutMicros)
{
    bool connected = false;
    int rc = connect(fd, (const struct sockaddr*) sin, sizeof(*sin));
    /* Capture errno immediately after connect so the EINPROGRESS test
     * below satisfies MISRA 22.10 - no intervening C-library calls
     * between the errno-setting function and the read. */
    int connectErrno = (rc < 0) ? errno : 0;

    if (rc == 0)
    {
        connected = true;
    }
    else if (connectErrno == EINPROGRESS)
    {
        connected =
            PosixTcpStream_WaitForConnectCompletion(fd, timeoutMicros) && PosixTcpStream_ReadDeferredConnectError(fd);
    }
    else if (PosixTcpStream_IsRemoteConnectError(connectErrno))
    {
        PosixTcpStream_ReportConnectFailureWithErrno(
            SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_REFUSED,
            connectErrno
        );
    }
    else
    {
        PosixTcpStream_ReportConnectFailureWithErrno(
            SOLIDSYSLOG_STREAM_CONNECT_LOCAL_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_NOT_STARTED,
            connectErrno
        );
    }
    return connected;
}

/* Remote means something was transmitted and the destination or the network
 * answered, or failed to: a reset came back, or a sent SYN drew nothing. Every
 * other immediate failure is local - routing rejected the call, or the stack
 * would not accept the socket - and nothing left this device, which is what
 * CONNECT_NOT_STARTED says. An unreachable report is local here because the
 * routing lookup failed before anything was sent; one that arrives after the
 * SYN is out reaches us through SO_ERROR instead, on the remote path. */
static inline bool PosixTcpStream_IsRemoteConnectError(int connectErrno)
{
    return (connectErrno == ECONNREFUSED) || (connectErrno == ETIMEDOUT);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- fd is a kernel file descriptor; timeoutMicros is a duration; distinct semantics
static bool PosixTcpStream_WaitForConnectCompletion(int fd, long timeoutMicros)
{
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(fd, &writeSet);

    fd_set errorSet;
    FD_ZERO(&errorSet);
    FD_SET(fd, &errorSet);

    struct timeval timeout = {.tv_sec = timeoutMicros / 1000000L, .tv_usec = timeoutMicros % 1000000L};

    int rc = select(fd + 1, NULL, &writeSet, &errorSet, &timeout);
    /* Capture errno immediately after select (MISRA 22.10). */
    int selectErrno = (rc < 0) ? errno : 0;
    bool ready = (rc > 0) && FD_ISSET(fd, &writeSet) && !FD_ISSET(fd, &errorSet);
    if (!ready)
    {
        PosixTcpStream_ReportUnfinishedWait(fd, rc, selectErrno);
    }
    return ready;
}

/* The budget expiring is nothing to report a code for. A select that failed
 * left its errno. A socket select answered for, flagged in the exception set,
 * holds the failure in SO_ERROR. */
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- a descriptor, a select result and the errno it left; distinct semantics
static void PosixTcpStream_ReportUnfinishedWait(int fd, int selectResult, int selectErrno)
{
    if (PosixTcpStream_WaitTimedOut(selectResult))
    {
        PosixTcpStream_ReportConnectFailure(
            SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_TIMED_OUT
        );
    }
    else
    {
        int nativeErrno = selectErrno;
        if (selectResult > 0)
        {
            (void) PosixTcpStream_ReadSocketError(fd, &nativeErrno);
        }
        PosixTcpStream_ReportConnectFailureWithErrno(
            SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_REFUSED,
            nativeErrno
        );
    }
}

/* select() returning zero is the budget expiring with nothing to report. Any
 * other unready outcome means the destination answered, just not with a
 * connection. */
static inline bool PosixTcpStream_WaitTimedOut(int selectResult)
{
    return selectResult == 0;
}

static bool PosixTcpStream_ReadDeferredConnectError(int fd)
{
    int code = 0;
    bool asked = PosixTcpStream_ReadSocketError(fd, &code);
    bool connected = asked && (code == 0);
    if (!connected)
    {
        PosixTcpStream_ReportConnectFailureWithErrno(
            SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_REFUSED,
            code
        );
    }
    return connected;
}

/* The error the socket holds, or, when it cannot be asked, the lookup's own:
 * true when the socket answered. */
static bool PosixTcpStream_ReadSocketError(int fd, int* code)
{
    int err = 0;
    socklen_t errlen = (socklen_t) sizeof(err);
    int rc = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
    /* Capture errno immediately after getsockopt (MISRA 22.10). */
    int lookupErrno = (rc != 0) ? errno : 0;
    *code = (rc == 0) ? err : lookupErrno;
    return rc == 0;
}

static bool PosixTcpStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size)
{
    struct SolidSyslogPosixTcpStream* self = PosixTcpStream_SelfFromBase(base);
    bool ok = PosixTcpStream_PeerIsStillThere(self);

    if (ok)
    {
        ssize_t sent = send(self->Fd, buffer, size, MSG_NOSIGNAL);
        ok = PosixTcpStream_WroteAllBytes(sent, size);
    }
    if (!ok)
    {
        PosixTcpStream_Close(base);
    }
    return ok;
}

/* A peer that has closed its end leaves the socket writable, so send takes the
 * record, and only the peer's reset - a round trip later - says it went
 * nowhere. By then Send has returned true and the record has left the caller's
 * hands. A FIN that has arrived reads as end-of-stream, so ask first: a
 * half-closed connection fails the send and store-and-forward replays the
 * record instead of losing it. Bytes the peer sent that nothing has read stand
 * in front of its FIN, and the peek sees those instead. They are left for Read
 * rather than discarded: a stream layered over this one may be mid-handshake,
 * with the peer's reply queued here. A syslog collector sends nothing back, so
 * the queue is empty when the check runs. */
static bool PosixTcpStream_PeerIsStillThere(struct SolidSyslogPosixTcpStream* self)
{
    char discard = 0;
    ssize_t peeked = recv(self->Fd, &discard, sizeof(discard), MSG_PEEK);
    /* Captured immediately after recv so the test below satisfies MISRA 22.10 -
     * no intervening library call between the errno-setting function and the
     * read. */
    int peekErrno = (peeked < 0) ? errno : 0;
    return (peeked > 0) || ((peeked < 0) && PosixTcpStream_WouldBlock(peekErrno));
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
    /* Capture errno immediately after recv so the WouldBlock test below
     * satisfies MISRA 22.10 - passing the captured value into the helper
     * keeps the predicate pure and decouples it from errno's lifetime. */
    int recvErrno = (n < 0) ? errno : 0;
    SolidSyslogSsize result = -1;

    if (n > 0)
    {
        result = (SolidSyslogSsize) n;
    }
    else if ((n < 0) && PosixTcpStream_WouldBlock(recvErrno))
    {
        result = 0;
    }
    else
    {
        PosixTcpStream_Close(base);
    }
    return result;
}

static inline bool PosixTcpStream_WouldBlock(int err)
{
    return (err == EAGAIN) || (err == EWOULDBLOCK);
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

static uint32_t PosixTcpStream_Version(struct SolidSyslogStream* base)
{
    (void) base;
    return 0U;
}
