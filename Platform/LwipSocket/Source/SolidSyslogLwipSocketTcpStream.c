/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

#if LWIP_SOCKET && LWIP_TCP

#include "lwip/errno.h"
#include "lwip/sockets.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipSocketAddressPrivate.h"
#include "SolidSyslogLwipSocketTcpStreamErrors.h"
#include "SolidSyslogLwipSocketTcpStreamPrivate.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogStreamCategories.h"
#include "SolidSyslogTunables.h"

const struct SolidSyslogErrorSource SolidSyslogLwipSocketTcpStreamErrorSource = {"LwipSocketTcpStream"};

struct SolidSyslogAddress;

enum
{
    INVALID_SOCKET = -1
};

static uint32_t LwipSocketTcpStream_NullConnectTimeoutGetter(void* context);
static inline bool LwipSocketTcpStream_ConfigProvidesGetter(const struct SolidSyslogLwipSocketTcpStreamConfig* config);

static bool LwipSocketTcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);
static bool LwipSocketTcpStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size);
static SolidSyslogSsize LwipSocketTcpStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size);
static void LwipSocketTcpStream_CloseSocket(struct SolidSyslogLwipSocketTcpStream* self);
static bool LwipSocketTcpStream_WroteAllBytes(ssize_t sent, size_t expected);
static inline bool LwipSocketTcpStream_WouldBlock(int err);

static inline struct SolidSyslogLwipSocketTcpStream* LwipSocketTcpStream_SelfFromBase(struct SolidSyslogStream* base);
static int LwipSocketTcpStream_TakeSocket(void);
static inline bool LwipSocketTcpStream_IsSocketValid(int fd);
static bool LwipSocketTcpStream_ConnectOrCloseOnFailure(
    struct SolidSyslogLwipSocketTcpStream* self,
    const struct sockaddr_in* sin
);
static bool LwipSocketTcpStream_Connect(struct SolidSyslogLwipSocketTcpStream* self, const struct sockaddr_in* sin);
static bool LwipSocketTcpStream_WaitForConnectCompletion(int fd, long timeoutMicros);
static long LwipSocketTcpStream_ResolveConnectTimeoutMicros(struct SolidSyslogLwipSocketTcpStream* self);
static bool LwipSocketTcpStream_ReadDeferredConnectError(int fd);
static void LwipSocketTcpStream_ReportConnectFailure(
    enum SolidSyslogSeverity severity,
    enum SolidSyslogTcpStreamErrors detail
);
static inline bool LwipSocketTcpStream_WaitTimedOut(int selectResult);
static inline bool LwipSocketTcpStream_IsRemoteConnectError(int connectErrno);

void SolidSyslogLwipSocketTcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogLwipSocketTcpStreamConfig* config
)
{
    struct SolidSyslogLwipSocketTcpStream* self = LwipSocketTcpStream_SelfFromBase(base);
    self->Base.Open = LwipSocketTcpStream_Open;
    self->Base.Send = LwipSocketTcpStream_Send;
    self->Base.Read = LwipSocketTcpStream_Read;
    self->Config.GetConnectTimeoutMs = LwipSocketTcpStream_NullConnectTimeoutGetter;
    self->Config.ConnectTimeoutContext = NULL;
    if (LwipSocketTcpStream_ConfigProvidesGetter(config) == true)
    {
        self->Config = *config;
    }
}

static inline bool LwipSocketTcpStream_ConfigProvidesGetter(const struct SolidSyslogLwipSocketTcpStreamConfig* config)
{
    return (config != NULL) && (config->GetConnectTimeoutMs != NULL);
}

/* Null Object substituted when the integrator installs no getter - the bounded
 * wait then has one code path whether or not runtime tuning was wired. */
static uint32_t LwipSocketTcpStream_NullConnectTimeoutGetter(void* context)
{
    (void) context;
    return (uint32_t) SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS;
}

void SolidSyslogLwipSocketTcpStream_Cleanup(struct SolidSyslogStream* base)
{
    /* Overwrite the abstract base with the shared NullStream vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullStream_Get();
}

static bool LwipSocketTcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr)
{
    struct SolidSyslogLwipSocketTcpStream* self = LwipSocketTcpStream_SelfFromBase(base);
    const struct sockaddr_in* sin = SolidSyslogLwipSocketAddress_AsConstSockaddrIn(addr);
    bool connected = false;

    self->Fd = LwipSocketTcpStream_TakeSocket();
    if (LwipSocketTcpStream_IsSocketValid(self->Fd))
    {
        connected = LwipSocketTcpStream_ConnectOrCloseOnFailure(self, sin);
    }
    else
    {
        LwipSocketTcpStream_ReportConnectFailure(
            SOLIDSYSLOG_STREAM_CONNECT_LOCAL_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_ENDPOINT_UNAVAILABLE
        );
    }
    return connected;
}

static inline struct SolidSyslogLwipSocketTcpStream* LwipSocketTcpStream_SelfFromBase(struct SolidSyslogStream* base)
{
    return (struct SolidSyslogLwipSocketTcpStream*) base;
}

/* Non-blocking from the start: the connect reports EINPROGRESS, the wait is
 * bounded, and Send and Read never block the service thread on a wedged peer
 * or a full send buffer. */
static int LwipSocketTcpStream_TakeSocket(void)
{
    int fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (LwipSocketTcpStream_IsSocketValid(fd))
    {
        (void) lwip_fcntl(fd, F_SETFL, O_NONBLOCK);
    }
    return fd;
}

static inline bool LwipSocketTcpStream_IsSocketValid(int fd)
{
    return fd >= 0;
}

/* A socket the connect did not finish with is of no use to anyone, and a pool
 * class that keeps one leaks it until Destroy. */
static bool LwipSocketTcpStream_ConnectOrCloseOnFailure(
    struct SolidSyslogLwipSocketTcpStream* self,
    const struct sockaddr_in* sin
)
{
    bool connected = LwipSocketTcpStream_Connect(self, sin);
    if (!connected)
    {
        LwipSocketTcpStream_CloseSocket(self);
    }
    return connected;
}

static bool LwipSocketTcpStream_Connect(
    struct SolidSyslogLwipSocketTcpStream* self,
    const struct sockaddr_in* sin
)
{
    int rc = lwip_connect(self->Fd, (const struct sockaddr*) sin, sizeof(*sin));
    /* Captured immediately after lwip_connect so the test below satisfies
     * MISRA 22.10 - no intervening library call between the errno-setting
     * function and the read. */
    int connectErrno = (rc < 0) ? errno : 0;
    bool connected = (rc == 0);

    if (connectErrno == EINPROGRESS)
    {
        connected = LwipSocketTcpStream_WaitForConnectCompletion(
                        self->Fd,
                        LwipSocketTcpStream_ResolveConnectTimeoutMicros(self)
                    ) &&
                    LwipSocketTcpStream_ReadDeferredConnectError(self->Fd);
    }
    else if (LwipSocketTcpStream_IsRemoteConnectError(connectErrno))
    {
        LwipSocketTcpStream_ReportConnectFailure(
            SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_REFUSED
        );
    }
    else if (rc < 0)
    {
        LwipSocketTcpStream_ReportConnectFailure(
            SOLIDSYSLOG_STREAM_CONNECT_LOCAL_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_NOT_STARTED
        );
    }
    else
    {
        /* connected outright - nothing to report */
    }
    return connected;
}

static bool LwipSocketTcpStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size)
{
    struct SolidSyslogLwipSocketTcpStream* self = LwipSocketTcpStream_SelfFromBase(base);
    ssize_t sent = lwip_send(self->Fd, buffer, size, 0);
    bool ok = LwipSocketTcpStream_WroteAllBytes(sent, size);

    if (!ok)
    {
        LwipSocketTcpStream_CloseSocket(self);
    }
    return ok;
}

/* Non-blocking single-call contract: a short write or any error means the
 * connection is gone; the sender reconnects on its next pass. */
static bool LwipSocketTcpStream_WroteAllBytes(ssize_t sent, size_t expected)
{
    return (sent >= 0) && ((size_t) sent == expected);
}

static SolidSyslogSsize LwipSocketTcpStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size)
{
    struct SolidSyslogLwipSocketTcpStream* self = LwipSocketTcpStream_SelfFromBase(base);
    ssize_t received = lwip_recv(self->Fd, buffer, size, 0);
    /* Captured immediately after lwip_recv so the test below satisfies
     * MISRA 22.10 - no intervening library call between the errno-setting
     * function and the read. */
    int recvErrno = (received < 0) ? errno : 0;
    SolidSyslogSsize result = -1;

    if (received > 0)
    {
        result = (SolidSyslogSsize) received;
    }
    else if ((received < 0) && LwipSocketTcpStream_WouldBlock(recvErrno))
    {
        result = 0;
    }
    else
    {
        LwipSocketTcpStream_CloseSocket(self);
    }
    return result;
}

/* lwIP maps both "nothing arrived yet" and the receive timeout onto
 * EWOULDBLOCK, and EAGAIN is the same value; neither says the connection is
 * gone, so the stream keeps it and the caller tries again. */
static inline bool LwipSocketTcpStream_WouldBlock(int err)
{
    return (err == EWOULDBLOCK) || (err == EAGAIN);
}

static void LwipSocketTcpStream_CloseSocket(struct SolidSyslogLwipSocketTcpStream* self)
{
    if (LwipSocketTcpStream_IsSocketValid(self->Fd))
    {
        (void) lwip_close(self->Fd);
        self->Fd = INVALID_SOCKET;
    }
}

/* Read on every attempt, so a runtime-tunable value takes effect on the next
 * reconnect. */
static long LwipSocketTcpStream_ResolveConnectTimeoutMicros(struct SolidSyslogLwipSocketTcpStream* self)
{
    uint32_t ms = self->Config.GetConnectTimeoutMs(self->Config.ConnectTimeoutContext);
    return (long) ms * 1000L;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- fd is a socket descriptor; timeoutMicros is a duration; distinct semantics
static bool LwipSocketTcpStream_WaitForConnectCompletion(int fd, long timeoutMicros)
{
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(fd, &writeSet);

    fd_set errorSet;
    FD_ZERO(&errorSet);
    FD_SET(fd, &errorSet);

    struct timeval timeout = {.tv_sec = timeoutMicros / 1000000L, .tv_usec = timeoutMicros % 1000000L};

    int rc = lwip_select(fd + 1, NULL, &writeSet, &errorSet, &timeout);
    bool ready = (rc > 0) && FD_ISSET(fd, &writeSet) && !FD_ISSET(fd, &errorSet);

    if (!ready)
    {
        LwipSocketTcpStream_ReportConnectFailure(
            SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
            LwipSocketTcpStream_WaitTimedOut(rc) ? SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_TIMED_OUT
                                                 : SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_REFUSED
        );
    }
    return ready;
}

/* Remote means the destination or the network answered, or failed to. Every
 * other immediate failure is local - the stack would not accept the socket, or
 * had no route or memory to start the attempt - and nothing left this device,
 * which is what CONNECT_NOT_STARTED says. */
static inline bool LwipSocketTcpStream_IsRemoteConnectError(int connectErrno)
{
    return (connectErrno == ECONNREFUSED) || (connectErrno == ETIMEDOUT);
}

/* lwip_select answering zero is the budget expiring with nothing to report.
 * Any other unready outcome means the destination answered, just not with a
 * connection. */
static inline bool LwipSocketTcpStream_WaitTimedOut(int selectResult)
{
    return selectResult == 0;
}

/* Writability alone does not mean connected: a non-blocking connect reports
 * its outcome through SO_ERROR, and a failed one becomes writable too. */
static bool LwipSocketTcpStream_ReadDeferredConnectError(int fd)
{
    int err = 0;
    socklen_t errlen = (socklen_t) sizeof(err);
    int rc = lwip_getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
    bool connected = (rc == 0) && (err == 0);

    if (!connected)
    {
        LwipSocketTcpStream_ReportConnectFailure(
            SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_REFUSED
        );
    }
    return connected;
}

/* Every connect failure is one category with the detail naming which step
 * failed, so a portable handler reacts to "no connection" without knowing
 * lwIP. Each failing step reports its own, because each is the only place that
 * knows which one it was. */
static void LwipSocketTcpStream_ReportConnectFailure(
    enum SolidSyslogSeverity severity,
    enum SolidSyslogTcpStreamErrors detail
)
{
    LwipSocketTcpStream_Report(severity, SOLIDSYSLOG_CAT_STREAM_CONNECT_FAILED, detail);
}

#else

/* ISO C forbids an empty translation unit. */
typedef int LwipSocketTcpStream_EmptyTranslationUnit;

#endif /* LWIP_SOCKET && LWIP_TCP */
