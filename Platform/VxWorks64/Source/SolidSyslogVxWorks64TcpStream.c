/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64TcpStream.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "vxWorks.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/times.h>

#include "ioLib.h"
#include "sockLib.h"

#include "SolidSyslogError.h"
#include "SolidSyslogStreamCategories.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogVxWorks64AddressPrivate.h"
#include "SolidSyslogVxWorks64TcpStreamErrors.h"
#include "SolidSyslogVxWorks64TcpStreamPrivate.h"

const struct SolidSyslogErrorSource SolidSyslogVxWorks64TcpStreamErrorSource = {"VxWorks64TcpStream"};

struct SolidSyslogAddress;

enum
{
    MILLISECONDS_PER_SECOND = 1000,
    MICROSECONDS_PER_MILLISECOND = 1000
};

static uint32_t VxWorks64TcpStream_NullConnectTimeoutGetter(void* context);
static inline bool VxWorks64TcpStream_ConfigProvidesGetter(const struct SolidSyslogVxWorks64TcpStreamConfig* config);

static bool VxWorks64TcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);

static inline struct SolidSyslogVxWorks64TcpStream* VxWorks64TcpStream_SelfFromBase(struct SolidSyslogStream* base);
static inline bool VxWorks64TcpStream_IsSocketValid(int fd);
static void VxWorks64TcpStream_ReportConnectFailure(
    enum SolidSyslogSeverity severity,
    enum SolidSyslogTcpStreamErrors detail
);
static bool VxWorks64TcpStream_ConnectOrCloseOnFailure(
    struct SolidSyslogVxWorks64TcpStream* self,
    int fd,
    const struct sockaddr_in* sin
);
static bool VxWorks64TcpStream_Connect(struct SolidSyslogVxWorks64TcpStream* self, int fd, const struct sockaddr_in* sin);
static void VxWorks64TcpStream_ApplySocketOptions(int fd);
static void VxWorks64TcpStream_ReportOptionRefused(void);
static struct timeval VxWorks64TcpStream_ResolveConnectTimeout(struct SolidSyslogVxWorks64TcpStream* self);
static inline bool VxWorks64TcpStream_IsRemoteConnectError(int connectErrno);

void SolidSyslogVxWorks64TcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogVxWorks64TcpStreamConfig* config
)
{
    struct SolidSyslogVxWorks64TcpStream* self = VxWorks64TcpStream_SelfFromBase(base);
    self->Base.Open = VxWorks64TcpStream_Open;
    self->Config.GetConnectTimeoutMs = VxWorks64TcpStream_NullConnectTimeoutGetter;
    self->Config.ConnectTimeoutContext = NULL;
    if (VxWorks64TcpStream_ConfigProvidesGetter(config) == true)
    {
        self->Config = *config;
    }
}

static inline bool VxWorks64TcpStream_ConfigProvidesGetter(const struct SolidSyslogVxWorks64TcpStreamConfig* config)
{
    return (config != NULL) && (config->GetConnectTimeoutMs != NULL);
}

/* Null Object substituted when the integrator installs no getter - the bounded
 * connect then has one code path whether or not runtime tuning was wired. */
static uint32_t VxWorks64TcpStream_NullConnectTimeoutGetter(void* context)
{
    (void) context;
    return (uint32_t) SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS;
}

static inline struct SolidSyslogVxWorks64TcpStream* VxWorks64TcpStream_SelfFromBase(struct SolidSyslogStream* base)
{
    return (struct SolidSyslogVxWorks64TcpStream*) base;
}

static bool VxWorks64TcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr)
{
    struct SolidSyslogVxWorks64TcpStream* self = VxWorks64TcpStream_SelfFromBase(base);
    const struct sockaddr_in* sin = SolidSyslogVxWorks64Address_AsConstSockaddrIn(addr);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    bool connected = false;
    if (VxWorks64TcpStream_IsSocketValid(fd))
    {
        connected = VxWorks64TcpStream_ConnectOrCloseOnFailure(self, fd, sin);
    }
    else
    {
        VxWorks64TcpStream_ReportConnectFailure(
            SOLIDSYSLOG_STREAM_CONNECT_LOCAL_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_ENDPOINT_UNAVAILABLE
        );
    }
    return connected;
}

static inline bool VxWorks64TcpStream_IsSocketValid(int fd)
{
    return fd >= 0;
}

/* Every connect failure is one category with the detail naming which step
 * failed, so a portable handler reacts to "no connection" without knowing the
 * stack. */
static void VxWorks64TcpStream_ReportConnectFailure(
    enum SolidSyslogSeverity severity,
    enum SolidSyslogTcpStreamErrors detail
)
{
    VxWorks64TcpStream_Report(severity, SOLIDSYSLOG_CAT_STREAM_CONNECT_FAILED, detail);
}

/* A socket the connect did not finish with is of no use to anyone, and a pool
 * class that keeps one leaks it until Destroy. */
static bool VxWorks64TcpStream_ConnectOrCloseOnFailure(
    struct SolidSyslogVxWorks64TcpStream* self,
    int fd,
    const struct sockaddr_in* sin
)
{
    bool connected = VxWorks64TcpStream_Connect(self, fd, sin);
    if (connected)
    {
        VxWorks64TcpStream_ApplySocketOptions(fd);
    }
    else
    {
        (void) close(fd);
    }
    return connected;
}

/* connectWithTimeout bounds the attempt itself and reports the bound expiring as
 * EINPROGRESS, so the service pass is never held longer than the getter allows.
 * An attempt still under way when it returns dies with the socket. */
static bool VxWorks64TcpStream_Connect(struct SolidSyslogVxWorks64TcpStream* self, int fd, const struct sockaddr_in* sin)
{
    struct timeval timeout = VxWorks64TcpStream_ResolveConnectTimeout(self);
    /* sockLib takes a non-const address that it only reads (D.006). */
    STATUS status = connectWithTimeout(fd, (struct sockaddr*) sin, (int) sizeof(*sin), &timeout);
    /* Read errno straight after the call that set it, with nothing between
     * (MISRA 22.10). */
    int connectErrno = (status == ERROR) ? errno : 0;
    bool connected = (status == OK);
    if (connectErrno == EINPROGRESS)
    {
        VxWorks64TcpStream_ReportConnectFailure(
            SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_TIMED_OUT
        );
    }
    else if (VxWorks64TcpStream_IsRemoteConnectError(connectErrno))
    {
        VxWorks64TcpStream_ReportConnectFailure(
            SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_REFUSED
        );
    }
    else if (!connected)
    {
        VxWorks64TcpStream_ReportConnectFailure(
            SOLIDSYSLOG_STREAM_CONNECT_LOCAL_SEVERITY,
            SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_NOT_STARTED
        );
    }
    else
    {
        /* connected - nothing to report */
    }
    return connected;
}

/* Latency first: a syslog record is small and waiting to coalesce it with the
 * next one only delays delivery. Keepalive then surfaces a peer that went away
 * during an idle period, rather than on the next record. Its timing is set for
 * the whole stack rather than per socket, so it is the integrator's to tune.
 * Both are set once the connection stands, because a refused option says the
 * connection is less robust than intended, which is only true once there is a
 * connection; a failed connect reports that instead. */
static void VxWorks64TcpStream_ApplySocketOptions(int fd)
{
    int enable = 1;
    bool allAccepted = (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*) &enable, (int) sizeof(enable)) == OK);
    allAccepted =
        (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char*) &enable, (int) sizeof(enable)) == OK) && allAccepted;
    if (!allAccepted)
    {
        VxWorks64TcpStream_ReportOptionRefused();
    }
}

/* One event per attempt however many options the stack declined: the
 * connection stands either way, and the engineer's next step is the same
 * whichever one it was. */
static void VxWorks64TcpStream_ReportOptionRefused(void)
{
    VxWorks64TcpStream_Report(
        SOLIDSYSLOG_SEVERITY_WARNING,
        SOLIDSYSLOG_CAT_STREAM_OPTION_REFUSED,
        SOLIDSYSLOG_TCP_STREAM_ERROR_SOCKET_OPTION_REFUSED
    );
}

/* Read on every attempt, so a runtime-tunable value takes effect on the next
 * reconnect. Any 32-bit count of milliseconds fits: its seconds stay below
 * 4294968, well inside the range C99 guarantees a long. */
static struct timeval VxWorks64TcpStream_ResolveConnectTimeout(struct SolidSyslogVxWorks64TcpStream* self)
{
    uint32_t ms = self->Config.GetConnectTimeoutMs(self->Config.ConnectTimeoutContext);
    struct timeval timeout;
    timeout.tv_sec = (long) (ms / (uint32_t) MILLISECONDS_PER_SECOND);
    timeout.tv_usec = (long) ((ms % (uint32_t) MILLISECONDS_PER_SECOND) * (uint32_t) MICROSECONDS_PER_MILLISECOND);
    return timeout;
}

/* Remote means something was transmitted and the destination or the network
 * answered, or failed to: a reset came back, or a sent SYN drew nothing. Every
 * other failure is local - the stack had no route or memory to start the
 * attempt - and nothing left this device, which is what CONNECT_NOT_STARTED
 * says. */
static inline bool VxWorks64TcpStream_IsRemoteConnectError(int connectErrno)
{
    return (connectErrno == ECONNREFUSED) || (connectErrno == ETIMEDOUT);
}
