/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

#if LWIP_SOCKET && LWIP_TCP

#include "lwip/errno.h"
#include "lwip/sockets.h"

#include <stdbool.h>
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

static uint32_t LwipSocketTcpStream_NullConnectTimeoutGetter(void* context);
static inline bool LwipSocketTcpStream_ConfigProvidesGetter(const struct SolidSyslogLwipSocketTcpStreamConfig* config);

static bool LwipSocketTcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);

static inline struct SolidSyslogLwipSocketTcpStream* LwipSocketTcpStream_SelfFromBase(struct SolidSyslogStream* base);
static bool LwipSocketTcpStream_WaitForConnectCompletion(int fd, long timeoutMicros);
static long LwipSocketTcpStream_ResolveConnectTimeoutMicros(struct SolidSyslogLwipSocketTcpStream* self);
static bool LwipSocketTcpStream_ReadDeferredConnectError(int fd);
static void LwipSocketTcpStream_ReportConnectFailure(
    enum SolidSyslogSeverity severity,
    enum SolidSyslogTcpStreamErrors detail
);

void SolidSyslogLwipSocketTcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogLwipSocketTcpStreamConfig* config
)
{
    struct SolidSyslogLwipSocketTcpStream* self = LwipSocketTcpStream_SelfFromBase(base);
    self->Base.Open = LwipSocketTcpStream_Open;
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
    self->Fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    (void) lwip_fcntl(self->Fd, F_SETFL, O_NONBLOCK);

    int rc = lwip_connect(self->Fd, (const struct sockaddr*) sin, sizeof(*sin));
    /* Captured immediately after lwip_connect so the test below satisfies
     * MISRA 22.10 - no intervening library call between the errno-setting
     * function and the read. */
    int connectErrno = (rc < 0) ? errno : 0;
    bool connected = true;

    if (connectErrno == EINPROGRESS)
    {
        connected = LwipSocketTcpStream_WaitForConnectCompletion(
                        self->Fd,
                        LwipSocketTcpStream_ResolveConnectTimeoutMicros(self)
                    ) &&
                    LwipSocketTcpStream_ReadDeferredConnectError(self->Fd);
    }
    return connected;
}

/* The connect is under way and the socket is non-blocking, so the answer comes
 * as writability. The exception set is watched alongside, because a stack that
 * ends the attempt reports it there rather than as a write. */
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
    bool ready = (rc > 0);

    if (!ready)
    {
        LwipSocketTcpStream_ReportConnectFailure(
            SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_TIMED_OUT
        );
    }
    return ready;
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

static inline struct SolidSyslogLwipSocketTcpStream* LwipSocketTcpStream_SelfFromBase(struct SolidSyslogStream* base)
{
    return (struct SolidSyslogLwipSocketTcpStream*) base;
}

#else

/* ISO C forbids an empty translation unit. */
typedef int LwipSocketTcpStream_EmptyTranslationUnit;

#endif /* LWIP_SOCKET && LWIP_TCP */
