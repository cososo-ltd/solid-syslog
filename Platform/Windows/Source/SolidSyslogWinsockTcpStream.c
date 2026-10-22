/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWinsockTcpStream.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <winsock2.h>
/* Needed for TCP_KEEPIDLE / TCP_KEEPINTVL / TCP_KEEPCNT below: the SDK defines
   them in <ws2ipdef.h>, which only <ws2tcpip.h> pulls in. No <ws2tcpip.h>
   symbol is named directly here, so the include reads as unused and is not. */
#include <ws2tcpip.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogStreamCategories.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWinsockAddressPrivate.h"
#include "SolidSyslogWinsockTcpStreamErrors.h"
#include "SolidSyslogWinsockTcpStreamInternal.h"
#include "SolidSyslogWinsockTcpStreamPrivate.h"

const struct SolidSyslogErrorSource SolidSyslogWinsockTcpStreamErrorSource = {
    "WinsockTcpStream",
    SolidSyslogTcpStreamErrors_Name
};

/* File-local forwarders. Taking the address of a __declspec(dllimport)
   Winsock function for static initialisation triggers MSVC C4232 (the address
   isn't a compile-time constant); forwarding through a static function whose
   address IS a compile-time constant avoids the warning without a suppression. */
static SOCKET WSAAPI WinsockTcpStream_CallSocket(int af, int type, int protocol);
static int WSAAPI WinsockTcpStream_CallConnect(SOCKET s, const struct sockaddr* name, int namelen);
static int WSAAPI WinsockTcpStream_CallSend(SOCKET s, const char* buf, int len, int flags);
static int WSAAPI WinsockTcpStream_CallRecv(SOCKET s, char* buf, int len, int flags);
static int WSAAPI WinsockTcpStream_CallSetSockOpt(SOCKET s, int level, int optname, const char* optval, int optlen);
static int WSAAPI WinsockTcpStream_CallGetSockOpt(SOCKET s, int level, int optname, char* optval, int* optlen);
static int WSAAPI WinsockTcpStream_CallCloseSocket(SOCKET s);
static int WSAAPI WinsockTcpStream_CallIoctlSocket(SOCKET s, long cmd, u_long* argp);
static int WSAAPI
CallSelect(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const struct timeval* timeout);
static int WSAAPI WinsockTcpStream_CallWSAGetLastError(void);

WinsockTcpStreamSocketFn WinsockTcpStream_socket = WinsockTcpStream_CallSocket;
WinsockTcpStreamConnectFn WinsockTcpStream_connect = WinsockTcpStream_CallConnect;
WinsockTcpStreamSendFn WinsockTcpStream_send = WinsockTcpStream_CallSend;
WinsockTcpStreamRecvFn WinsockTcpStream_recv = WinsockTcpStream_CallRecv;
WinsockTcpStreamSetSockOptFn WinsockTcpStream_setsockopt = WinsockTcpStream_CallSetSockOpt;
WinsockTcpStreamGetSockOptFn WinsockTcpStream_getsockopt = WinsockTcpStream_CallGetSockOpt;
WinsockTcpStreamCloseSocketFn WinsockTcpStream_closesocket = WinsockTcpStream_CallCloseSocket;
WinsockTcpStreamIoctlSocketFn WinsockTcpStream_ioctlsocket = WinsockTcpStream_CallIoctlSocket;
WinsockTcpStreamSelectFn WinsockTcpStream_select = CallSelect;
WinsockTcpStreamWSAGetLastErrorFn WinsockTcpStream_WSAGetLastError = WinsockTcpStream_CallWSAGetLastError;

static SOCKET WSAAPI WinsockTcpStream_CallSocket(int af, int type, int protocol)
{
    return socket(af, type, protocol);
}

static int WSAAPI WinsockTcpStream_CallConnect(SOCKET s, const struct sockaddr* name, int namelen)
{
    return connect(s, name, namelen);
}

static int WSAAPI WinsockTcpStream_CallSend(SOCKET s, const char* buf, int len, int flags)
{
    return send(s, buf, len, flags);
}

static int WSAAPI WinsockTcpStream_CallRecv(SOCKET s, char* buf, int len, int flags)
{
    return recv(s, buf, len, flags);
}

static int WSAAPI WinsockTcpStream_CallSetSockOpt(SOCKET s, int level, int optname, const char* optval, int optlen)
{
    return setsockopt(s, level, optname, optval, optlen);
}

static int WSAAPI WinsockTcpStream_CallGetSockOpt(SOCKET s, int level, int optname, char* optval, int* optlen)
{
    return getsockopt(s, level, optname, optval, optlen);
}

static int WSAAPI WinsockTcpStream_CallCloseSocket(SOCKET s)
{
    return closesocket(s);
}

static int WSAAPI WinsockTcpStream_CallIoctlSocket(SOCKET s, long cmd, u_long* argp)
{
    return ioctlsocket(s, cmd, argp);
}

static int WSAAPI
CallSelect(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const struct timeval* timeout)
{
    /* Winsock select() takes a non-const timeval*; const-cast keeps our seam
       signature const-correct for callers without forcing them to drop const. */
    return select(nfds, readfds, writefds, exceptfds, (struct timeval*) timeout);
}

static int WSAAPI WinsockTcpStream_CallWSAGetLastError(void)
{
    return WSAGetLastError();
}

enum
{
    MILLISECONDS_PER_SECOND = 1000,
    MICROSECONDS_PER_MILLISECOND = 1000,
    /* Winsock ignores nfds (its fd_set is a literal array, not a bitmask),
       but POSIX-portable callers must pass the highest fd + 1. Pass any
       positive value to keep the call well-formed against either ABI. */
    WINSOCK_NFDS_IGNORED = 1
};

static uint32_t WinsockTcpStream_NullConnectTimeoutGetter(void* context);

static bool WinsockTcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);
static bool WinsockTcpStream_TakeSocketAndConnect(
    struct SolidSyslogWinsockTcpStream* self,
    const struct sockaddr_in* sin
);
static bool WinsockTcpStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size);
static SolidSyslogSsize WinsockTcpStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size);
static void WinsockTcpStream_Close(struct SolidSyslogStream* base);
static uint32_t WinsockTcpStream_Version(struct SolidSyslogStream* base);

static inline struct SolidSyslogWinsockTcpStream* WinsockTcpStream_SelfFromBase(struct SolidSyslogStream* base);
static inline bool WinsockTcpStream_ConfigProvidesGetter(const struct SolidSyslogWinsockTcpStreamConfig* config);
static void WinsockTcpStream_ReportConnectFailureWithLastError(
    enum SolidSyslogSeverity severity,
    enum SolidSyslogTcpStreamErrors detail,
    int lastError
);
static void WinsockTcpStream_ReportUnfinishedWait(SOCKET fd, int selectResult, int selectLastError);

static SOCKET WinsockTcpStream_OpenAndConfigureSocket(void);
static bool WinsockTcpStream_ConfigureSocket(SOCKET fd);
static void WinsockTcpStream_EnableTcpNoDelay(SOCKET fd);
static void WinsockTcpStream_EnableKeepalive(SOCKET fd);
static inline bool WinsockTcpStream_IsSocketValid(SOCKET fd);
static bool WinsockTcpStream_ConnectOrCloseOnFailure(
    struct SolidSyslogWinsockTcpStream* self,
    const struct sockaddr_in* sin
);
static bool WinsockTcpStream_Connect(SOCKET fd, const struct sockaddr_in* sin, uint32_t connectTimeoutMs);
static bool WinsockTcpStream_SetNonBlocking(SOCKET fd);
static bool WinsockTcpStream_WaitForConnectCompletion(SOCKET fd, uint32_t connectTimeoutMs);
static bool WinsockTcpStream_ReadDeferredConnectError(SOCKET fd);
static bool WinsockTcpStream_ReadSocketError(SOCKET fd, int* code);
static void WinsockTcpStream_ReportConnectFailure(
    enum SolidSyslogSeverity severity,
    enum SolidSyslogTcpStreamErrors detail
);
static inline bool WinsockTcpStream_IsRemoteConnectError(int wsaError);
static inline bool WinsockTcpStream_WaitTimedOut(int selectResult);
static uint32_t WinsockTcpStream_ResolveConnectTimeoutMs(struct SolidSyslogWinsockTcpStream* self);
static bool WinsockTcpStream_PeerIsStillThere(struct SolidSyslogWinsockTcpStream* self);
static bool WinsockTcpStream_WroteAllBytes(int sent, size_t expected);
static inline bool WinsockTcpStream_WouldBlock(int wsaError);

void SolidSyslogWinsockTcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogWinsockTcpStreamConfig* config
)
{
    static const struct SolidSyslogWinsockTcpStream DefaultWinsockTcpStream = {
        .Base =
            {.Open = WinsockTcpStream_Open,
             .Send = WinsockTcpStream_Send,
             .Read = WinsockTcpStream_Read,
             .Close = WinsockTcpStream_Close,
             .Version = WinsockTcpStream_Version},
        .Config = {.GetConnectTimeoutMs = WinsockTcpStream_NullConnectTimeoutGetter, .ConnectTimeoutContext = NULL},
        .Fd = INVALID_SOCKET,
    };

    struct SolidSyslogWinsockTcpStream* self = WinsockTcpStream_SelfFromBase(base);
    *self = DefaultWinsockTcpStream;
    if (WinsockTcpStream_ConfigProvidesGetter(config) == true)
    {
        self->Config = *config;
    }
}

static inline bool WinsockTcpStream_ConfigProvidesGetter(const struct SolidSyslogWinsockTcpStreamConfig* config)
{
    return (config != NULL) && (config->GetConnectTimeoutMs != NULL);
}

/* Null Object substituted when the integrator does not install a getter -
 * returns the compile-time tunable so the bounded-wait path has a single
 * code path regardless of whether the integrator wired runtime tuning. */
static uint32_t WinsockTcpStream_NullConnectTimeoutGetter(void* context)
{
    (void) context;
    return (uint32_t) SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS;
}

static inline struct SolidSyslogWinsockTcpStream* WinsockTcpStream_SelfFromBase(struct SolidSyslogStream* base)
{
    return (struct SolidSyslogWinsockTcpStream*) base;
}

void SolidSyslogWinsockTcpStream_Cleanup(struct SolidSyslogStream* base)
{
    WinsockTcpStream_Close(base);
    /* Overwrite the abstract base with the shared NullStream vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullStream_Get();
}

static void WinsockTcpStream_Close(struct SolidSyslogStream* base)
{
    struct SolidSyslogWinsockTcpStream* self = WinsockTcpStream_SelfFromBase(base);
    if (WinsockTcpStream_IsSocketValid(self->Fd))
    {
        WinsockTcpStream_closesocket(self->Fd);
        self->Fd = INVALID_SOCKET;
    }
}

static uint32_t WinsockTcpStream_Version(struct SolidSyslogStream* base)
{
    (void) base;
    return 0U;
}

static bool WinsockTcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr)
{
    struct SolidSyslogWinsockTcpStream* self = WinsockTcpStream_SelfFromBase(base);
    bool connected = false;
    if (WinsockTcpStream_IsSocketValid(self->Fd))
    {
        /* Refused by contract, not reconnected: what is held stays as it is. */
        WinsockTcpStream_Report(
            SOLIDSYSLOG_BAD_ARGUMENT_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_ARGUMENT,
            SOLIDSYSLOG_TCP_STREAM_ERROR_ALREADY_OPEN
        );
    }
    else
    {
        connected = WinsockTcpStream_TakeSocketAndConnect(self, SolidSyslogWinsockAddress_AsConstSockaddrIn(addr));
    }
    return connected;
}

static bool WinsockTcpStream_TakeSocketAndConnect(
    struct SolidSyslogWinsockTcpStream* self,
    const struct sockaddr_in* sin
)
{
    bool connected = false;
    self->Fd = WinsockTcpStream_OpenAndConfigureSocket();
    if (WinsockTcpStream_IsSocketValid(self->Fd))
    {
        connected = WinsockTcpStream_ConnectOrCloseOnFailure(self, sin);
    }
    return connected;
}

/* Every connect failure is one category with the detail naming which step
 * failed, so a portable handler reacts to "no connection" without knowing
 * Winsock. Each failing step reports its own, because each is the only place
 * that knows which one it was, and the only place its WSAGetLastError can be
 * read before another call replaces it. */
static void WinsockTcpStream_ReportConnectFailure(
    enum SolidSyslogSeverity severity,
    enum SolidSyslogTcpStreamErrors detail
)
{
    WinsockTcpStream_Report(severity, SOLIDSYSLOG_CAT_STREAM_CONNECT_FAILED, detail);
}

/* The fault, with the Winsock error code behind it when the call left one. */
static void WinsockTcpStream_ReportConnectFailureWithLastError(
    enum SolidSyslogSeverity severity,
    enum SolidSyslogTcpStreamErrors detail,
    int lastError
)
{
    if (lastError != 0)
    {
        SolidSyslog_ErrorWithNativeCode(
            severity,
            &SolidSyslogWinsockTcpStreamErrorSource,
            SOLIDSYSLOG_CAT_STREAM_CONNECT_FAILED,
            (int32_t) detail,
            (int32_t) lastError
        );
    }
    else
    {
        WinsockTcpStream_ReportConnectFailure(severity, detail);
    }
}

/* Non-blocking from the start: connect() returns WSAEWOULDBLOCK and the wait
 * is bounded via select(); WinsockTcpStream_Send/WinsockTcpStream_Read never block the service thread on a
 * wedged peer or a full kernel send buffer. */
static SOCKET WinsockTcpStream_OpenAndConfigureSocket(void)
{
    SOCKET fd = WinsockTcpStream_socket(AF_INET, SOCK_STREAM, 0);
    /* Read the error once, immediately after socket, before any call replaces it. */
    int socketLastError = WinsockTcpStream_IsSocketValid(fd) ? 0 : WinsockTcpStream_WSAGetLastError();
    if (!WinsockTcpStream_IsSocketValid(fd))
    {
        WinsockTcpStream_ReportConnectFailureWithLastError(
            SOLIDSYSLOG_STREAM_CONNECT_LOCAL_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_ENDPOINT_UNAVAILABLE,
            socketLastError
        );
    }
    else if (!WinsockTcpStream_ConfigureSocket(fd))
    {
        WinsockTcpStream_closesocket(fd);
        fd = INVALID_SOCKET;
    }
    else
    {
        /* configured - ready to connect */
    }
    return fd;
}

static bool WinsockTcpStream_ConfigureSocket(SOCKET fd)
{
    WinsockTcpStream_EnableTcpNoDelay(fd);
    WinsockTcpStream_EnableKeepalive(fd);
    return WinsockTcpStream_SetNonBlocking(fd);
}

static void WinsockTcpStream_EnableTcpNoDelay(SOCKET fd)
{
    int enable = 1;
    WinsockTcpStream_setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*) &enable, (int) sizeof(enable));
}

/* TCP_KEEPIDLE / TCP_KEEPINTVL / TCP_KEEPCNT come from <ws2ipdef.h> via
 * <ws2tcpip.h>, and Windows 10 1709+ accepts them through setsockopt, so idle,
 * interval and count are each set directly. No TCP_USER_TIMEOUT analogue. */
static void WinsockTcpStream_EnableKeepalive(SOCKET fd)
{
    int enable = 1;
    int idle = (int) SOLIDSYSLOG_TCP_KEEPALIVE_IDLE_SECONDS;
    int interval = (int) SOLIDSYSLOG_TCP_KEEPALIVE_INTERVAL_SECONDS;
    int count = (int) SOLIDSYSLOG_TCP_KEEPALIVE_PROBE_COUNT;

    WinsockTcpStream_setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char*) &enable, (int) sizeof(enable));
    WinsockTcpStream_setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, (const char*) &idle, (int) sizeof(idle));
    WinsockTcpStream_setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, (const char*) &interval, (int) sizeof(interval));
    WinsockTcpStream_setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, (const char*) &count, (int) sizeof(count));
}

static inline bool WinsockTcpStream_IsSocketValid(SOCKET fd)
{
    return fd != INVALID_SOCKET;
}

static bool WinsockTcpStream_ConnectOrCloseOnFailure(
    struct SolidSyslogWinsockTcpStream* self,
    const struct sockaddr_in* sin
)
{
    uint32_t connectTimeoutMs = WinsockTcpStream_ResolveConnectTimeoutMs(self);
    bool connected = WinsockTcpStream_Connect(self->Fd, sin, connectTimeoutMs);
    if (!connected)
    {
        WinsockTcpStream_closesocket(self->Fd);
        self->Fd = INVALID_SOCKET;
    }
    return connected;
}

/* Bridges the integrator-installed getter (or the Null Object substituted in
 * Initialise) to the bounded select() deadline. Invoked on every connect
 * attempt so a runtime-tunable value takes effect on the next reconnect. */
static uint32_t WinsockTcpStream_ResolveConnectTimeoutMs(struct SolidSyslogWinsockTcpStream* self)
{
    uint32_t ms = self->Config.GetConnectTimeoutMs(self->Config.ConnectTimeoutContext);
    return (ms > SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS_MAX) ? SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS_MAX : ms;
}

/* Non-blocking connect with bounded wait. Windows' default blocking connect()
 * to a refused loopback port retries internally for ~2 s before returning
 * WSAECONNREFUSED - slow enough that the BlockStore service thread's drain
 * rate during an outage is throttled to ~0.5 records/s, preventing the
 * discard policy from firing in BDD outage scenarios. The non-blocking path
 * bounds each connect attempt to CONNECT_TIMEOUT_MILLISECONDS; the socket
 * stays non-blocking thereafter so WinsockTcpStream_Send/WinsockTcpStream_Read are also fail-fast. */
static bool WinsockTcpStream_Connect(SOCKET fd, const struct sockaddr_in* sin, uint32_t connectTimeoutMs)
{
    bool connected = false;
    int rc = WinsockTcpStream_connect(fd, (const struct sockaddr*) sin, (int) sizeof(*sin));
    /* Read the error once, immediately after connect, so both tests below see
     * the same value and no intervening call can replace it. */
    int lastError = (rc == SOCKET_ERROR) ? WinsockTcpStream_WSAGetLastError() : 0;

    if (rc != SOCKET_ERROR)
    {
        connected = true;
    }
    else if (lastError == WSAEWOULDBLOCK)
    {
        connected = WinsockTcpStream_WaitForConnectCompletion(fd, connectTimeoutMs) &&
                    WinsockTcpStream_ReadDeferredConnectError(fd);
    }
    else if (WinsockTcpStream_IsRemoteConnectError(lastError))
    {
        WinsockTcpStream_ReportConnectFailureWithLastError(
            SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_REFUSED,
            lastError
        );
    }
    else
    {
        WinsockTcpStream_ReportConnectFailureWithLastError(
            SOLIDSYSLOG_STREAM_CONNECT_LOCAL_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_NOT_STARTED,
            lastError
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
static inline bool WinsockTcpStream_IsRemoteConnectError(int wsaError)
{
    return (wsaError == WSAECONNREFUSED) || (wsaError == WSAETIMEDOUT);
}

/* A socket Winsock will not make non-blocking is of no use: the bounded
 * connect, and Send and Read never blocking the service thread, all rest on
 * it. The attempt is given up here, and this is the step that reports it. */
static bool WinsockTcpStream_SetNonBlocking(SOCKET fd)
{
    u_long mode = 1;
    int rc = WinsockTcpStream_ioctlsocket(fd, (long) FIONBIO, &mode);
    int ioctlLastError = (rc == SOCKET_ERROR) ? WinsockTcpStream_WSAGetLastError() : 0;
    bool nonBlocking = (rc != SOCKET_ERROR);
    if (!nonBlocking)
    {
        WinsockTcpStream_ReportConnectFailureWithLastError(
            SOLIDSYSLOG_STREAM_CONNECT_LOCAL_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_ENDPOINT_UNAVAILABLE,
            ioctlLastError
        );
    }
    return nonBlocking;
}

static bool WinsockTcpStream_WaitForConnectCompletion(SOCKET fd, uint32_t connectTimeoutMs)
{
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(fd, &writeSet);

    fd_set errorSet;
    FD_ZERO(&errorSet);
    FD_SET(fd, &errorSet);

    struct timeval timeout = {
        .tv_sec = (long) (connectTimeoutMs / MILLISECONDS_PER_SECOND),
        .tv_usec = (long) ((connectTimeoutMs % MILLISECONDS_PER_SECOND) * MICROSECONDS_PER_MILLISECOND)
    };

    int rc = WinsockTcpStream_select(WINSOCK_NFDS_IGNORED, NULL, &writeSet, &errorSet, &timeout);
    int selectLastError = (rc == SOCKET_ERROR) ? WinsockTcpStream_WSAGetLastError() : 0;
    bool ready = (rc > 0) && FD_ISSET(fd, &writeSet) && !FD_ISSET(fd, &errorSet);
    if (!ready)
    {
        WinsockTcpStream_ReportUnfinishedWait(fd, rc, selectLastError);
    }
    return ready;
}

/* The budget expiring is nothing to report a code for. A select that failed
 * left its error. A socket select answered for, flagged in the exception set,
 * is how Winsock reports a refused non-blocking connect, and the socket holds
 * the failure in SO_ERROR. */
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- a socket, a select result and the error it left; distinct semantics
static void WinsockTcpStream_ReportUnfinishedWait(SOCKET fd, int selectResult, int selectLastError)
{
    if (WinsockTcpStream_WaitTimedOut(selectResult))
    {
        WinsockTcpStream_ReportConnectFailure(
            SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_TIMED_OUT
        );
    }
    else
    {
        int lastError = selectLastError;
        if (selectResult > 0)
        {
            (void) WinsockTcpStream_ReadSocketError(fd, &lastError);
        }
        WinsockTcpStream_ReportConnectFailureWithLastError(
            SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_REFUSED,
            lastError
        );
    }
}

/* select() returning zero is the budget expiring with nothing to report. Any
 * other unready outcome means the destination answered, just not with a
 * connection. */
static inline bool WinsockTcpStream_WaitTimedOut(int selectResult)
{
    return selectResult == 0;
}

static bool WinsockTcpStream_ReadDeferredConnectError(SOCKET fd)
{
    int code = 0;
    bool asked = WinsockTcpStream_ReadSocketError(fd, &code);
    bool connected = asked && (code == 0);
    if (!connected)
    {
        WinsockTcpStream_ReportConnectFailureWithLastError(
            SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_REFUSED,
            code
        );
    }
    return connected;
}

/* The error the socket holds, or, when it cannot be asked, the lookup's own:
 * true when the socket answered. */
static bool WinsockTcpStream_ReadSocketError(SOCKET fd, int* code)
{
    int err = 0;
    int errlen = (int) sizeof(err);
    int rc = WinsockTcpStream_getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*) &err, &errlen);
    int lookupLastError = (rc == SOCKET_ERROR) ? WinsockTcpStream_WSAGetLastError() : 0;
    *code = (rc != SOCKET_ERROR) ? err : lookupLastError;
    return rc != SOCKET_ERROR;
}

static bool WinsockTcpStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size)
{
    struct SolidSyslogWinsockTcpStream* self = WinsockTcpStream_SelfFromBase(base);
    bool ok = WinsockTcpStream_PeerIsStillThere(self);

    if (ok)
    {
        int sent = WinsockTcpStream_send(self->Fd, (const char*) buffer, (int) size, 0);
        ok = WinsockTcpStream_WroteAllBytes(sent, size);
    }
    if (!ok)
    {
        WinsockTcpStream_Close(base);
    }
    return ok;
}

/* A peer that has closed its end leaves the socket writable, so send takes the
 * record, and only the peer's reset - a round trip later - says it went
 * nowhere. By then Send has returned true and the record has left the caller's
 * hands. A graceful close that has arrived reads as end-of-stream, so ask
 * first: a half-closed connection fails the send and store-and-forward replays
 * the record instead of losing it. */
static bool WinsockTcpStream_PeerIsStillThere(struct SolidSyslogWinsockTcpStream* self)
{
    char discard = 0;
    int peeked = WinsockTcpStream_recv(self->Fd, &discard, (int) sizeof(discard), MSG_PEEK);
    return (peeked > 0) ||
           ((peeked == SOCKET_ERROR) && WinsockTcpStream_WouldBlock(WinsockTcpStream_WSAGetLastError()));
}

/* Non-blocking single-call contract: short write or any error means the
 * connection is gone; the caller closes and reconnects on the next attempt. */
static bool WinsockTcpStream_WroteAllBytes(int sent, size_t expected)
{
    return (sent >= 0) && ((size_t) sent == expected);
}

static SolidSyslogSsize WinsockTcpStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size)
{
    struct SolidSyslogWinsockTcpStream* self = WinsockTcpStream_SelfFromBase(base);
    int n = WinsockTcpStream_recv(self->Fd, (char*) buffer, (int) size, 0);
    SolidSyslogSsize result = -1;

    if (n > 0)
    {
        result = (SolidSyslogSsize) n;
    }
    else if ((n == SOCKET_ERROR) && WinsockTcpStream_WouldBlock(WinsockTcpStream_WSAGetLastError()))
    {
        result = 0;
    }
    else
    {
        WinsockTcpStream_Close(base);
    }
    return result;
}

static inline bool WinsockTcpStream_WouldBlock(int wsaError)
{
    return wsaError == WSAEWOULDBLOCK;
}
