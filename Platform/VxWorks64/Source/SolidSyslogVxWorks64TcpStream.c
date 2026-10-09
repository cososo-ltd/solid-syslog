/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64TcpStream.h"

#include <errno.h>
#include <limits.h>
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
#include "SolidSyslogNullStream.h"
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
    INVALID_SOCKET = -1,
    MILLISECONDS_PER_SECOND = 1000,
    MICROSECONDS_PER_MILLISECOND = 1000
};

static uint32_t VxWorks64TcpStream_NullConnectTimeoutGetter(void* context);
static inline bool VxWorks64TcpStream_ConfigProvidesGetter(const struct SolidSyslogVxWorks64TcpStreamConfig* config);

static bool VxWorks64TcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);
static bool VxWorks64TcpStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size);
static inline bool VxWorks64TcpStream_FitsTheStack(size_t size);
static bool VxWorks64TcpStream_PeerIsStillThere(struct SolidSyslogVxWorks64TcpStream* self);
static inline bool VxWorks64TcpStream_WouldBlock(int err);
static inline bool VxWorks64TcpStream_WroteAllBytes(int sent, size_t expected);
static SolidSyslogSsize VxWorks64TcpStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size);
static inline int VxWorks64TcpStream_ClampToTheStack(size_t size);
static void VxWorks64TcpStream_Close(struct SolidSyslogStream* base);
static uint32_t VxWorks64TcpStream_Version(struct SolidSyslogStream* base);

static inline struct SolidSyslogVxWorks64TcpStream* VxWorks64TcpStream_SelfFromBase(struct SolidSyslogStream* base);
static void VxWorks64TcpStream_CloseSocket(struct SolidSyslogVxWorks64TcpStream* self);
static inline bool VxWorks64TcpStream_IsSocketValid(int fd);
static void VxWorks64TcpStream_ReportConnectFailure(
    enum SolidSyslogSeverity severity,
    enum SolidSyslogTcpStreamErrors detail
);
static bool VxWorks64TcpStream_ConnectOrCloseOnFailure(
    struct SolidSyslogVxWorks64TcpStream* self,
    const struct sockaddr_in* sin
);
static bool VxWorks64TcpStream_Connect(struct SolidSyslogVxWorks64TcpStream* self, const struct sockaddr_in* sin);
static struct timeval VxWorks64TcpStream_ResolveConnectTimeout(struct SolidSyslogVxWorks64TcpStream* self);
static inline bool VxWorks64TcpStream_IsRemoteConnectError(int connectErrno);
static void VxWorks64TcpStream_ApplySocketOptions(int fd);
static void VxWorks64TcpStream_ReportOptionRefused(void);

void SolidSyslogVxWorks64TcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogVxWorks64TcpStreamConfig* config
)
{
    struct SolidSyslogVxWorks64TcpStream* self = VxWorks64TcpStream_SelfFromBase(base);
    self->Base.Open = VxWorks64TcpStream_Open;
    self->Base.Send = VxWorks64TcpStream_Send;
    self->Base.Read = VxWorks64TcpStream_Read;
    self->Base.Close = VxWorks64TcpStream_Close;
    self->Base.Version = VxWorks64TcpStream_Version;
    self->Fd = INVALID_SOCKET;
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

/* Substituted when no getter is installed, so the connect has one code path. */
static uint32_t VxWorks64TcpStream_NullConnectTimeoutGetter(void* context)
{
    (void) context;
    return (uint32_t) SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS;
}

static inline struct SolidSyslogVxWorks64TcpStream* VxWorks64TcpStream_SelfFromBase(struct SolidSyslogStream* base)
{
    return (struct SolidSyslogVxWorks64TcpStream*) base;
}

void SolidSyslogVxWorks64TcpStream_Cleanup(struct SolidSyslogStream* base)
{
    VxWorks64TcpStream_CloseSocket(VxWorks64TcpStream_SelfFromBase(base));
    /* Use-after-destroy lands on the NullStream vtable. */
    *base = *SolidSyslogNullStream_Get();
}

static bool VxWorks64TcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr)
{
    struct SolidSyslogVxWorks64TcpStream* self = VxWorks64TcpStream_SelfFromBase(base);
    const struct sockaddr_in* sin = SolidSyslogVxWorks64Address_AsConstSockaddrIn(addr);
    bool connected = false;
    /* Reopening must not leak the descriptor already held. */
    VxWorks64TcpStream_CloseSocket(self);
    self->Fd = socket(AF_INET, SOCK_STREAM, 0);
    if (VxWorks64TcpStream_IsSocketValid(self->Fd))
    {
        connected = VxWorks64TcpStream_ConnectOrCloseOnFailure(self, sin);
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

static void VxWorks64TcpStream_CloseSocket(struct SolidSyslogVxWorks64TcpStream* self)
{
    if (VxWorks64TcpStream_IsSocketValid(self->Fd))
    {
        (void) close(self->Fd);
        self->Fd = INVALID_SOCKET;
    }
}

static inline bool VxWorks64TcpStream_IsSocketValid(int fd)
{
    return fd >= 0;
}

/* One category for every connect failure; the detail names the step. */
static void VxWorks64TcpStream_ReportConnectFailure(
    enum SolidSyslogSeverity severity,
    enum SolidSyslogTcpStreamErrors detail
)
{
    VxWorks64TcpStream_Report(severity, SOLIDSYSLOG_CAT_STREAM_CONNECT_FAILED, detail);
}

/* A failed connect closes its socket, which would otherwise be held until
 * Destroy. */
static bool VxWorks64TcpStream_ConnectOrCloseOnFailure(
    struct SolidSyslogVxWorks64TcpStream* self,
    const struct sockaddr_in* sin
)
{
    bool connected = VxWorks64TcpStream_Connect(self, sin);
    if (connected)
    {
        VxWorks64TcpStream_ApplySocketOptions(self->Fd);
    }
    else
    {
        VxWorks64TcpStream_CloseSocket(self);
    }
    return connected;
}

/* connectWithTimeout reports the bound expiring as EINPROGRESS. An attempt
 * still under way when it returns dies with the socket. */
static bool VxWorks64TcpStream_Connect(struct SolidSyslogVxWorks64TcpStream* self, const struct sockaddr_in* sin)
{
    struct timeval timeout = VxWorks64TcpStream_ResolveConnectTimeout(self);
    /* sockLib takes a non-const address that it only reads (D.006). */
    STATUS status = connectWithTimeout(self->Fd, (struct sockaddr*) sin, (int) sizeof(*sin), &timeout);
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

/* Any 32-bit count of milliseconds fits: its seconds stay below 4294968,
 * inside the range C99 guarantees a long. */
static struct timeval VxWorks64TcpStream_ResolveConnectTimeout(struct SolidSyslogVxWorks64TcpStream* self)
{
    uint32_t ms = self->Config.GetConnectTimeoutMs(self->Config.ConnectTimeoutContext);
    uint32_t seconds = ms / (uint32_t) MILLISECONDS_PER_SECOND;
    uint32_t microseconds = (ms % (uint32_t) MILLISECONDS_PER_SECOND) * (uint32_t) MICROSECONDS_PER_MILLISECOND;
    struct timeval timeout;
    timeout.tv_sec = (long) seconds;
    timeout.tv_usec = (long) microseconds;
    return timeout;
}

/* Remote: a SYN was sent and drew a reset or nothing. Any other failure is
 * local, and nothing left this device. */
static inline bool VxWorks64TcpStream_IsRemoteConnectError(int connectErrno)
{
    return (connectErrno == ECONNREFUSED) || (connectErrno == ETIMEDOUT);
}

/* NODELAY stops a small record waiting to coalesce; keepalive detects a peer
 * lost while idle. Set after connecting, so a refusal is reported only on a
 * connection that stands. */
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

/* One event per attempt, however many options were refused. */
static void VxWorks64TcpStream_ReportOptionRefused(void)
{
    VxWorks64TcpStream_Report(
        SOLIDSYSLOG_SEVERITY_WARNING,
        SOLIDSYSLOG_CAT_STREAM_OPTION_REFUSED,
        SOLIDSYSLOG_TCP_STREAM_ERROR_SOCKET_OPTION_REFUSED
    );
}

static bool VxWorks64TcpStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size)
{
    struct SolidSyslogVxWorks64TcpStream* self = VxWorks64TcpStream_SelfFromBase(base);
    bool ok = VxWorks64TcpStream_FitsTheStack(size) && VxWorks64TcpStream_PeerIsStillThere(self);
    if (ok)
    {
        int sent = send(self->Fd, (const char*) buffer, (int) size, MSG_DONTWAIT);
        ok = VxWorks64TcpStream_WroteAllBytes(sent, size);
    }
    if (!ok)
    {
        VxWorks64TcpStream_CloseSocket(self);
    }
    return ok;
}

/* The stack takes a record's length as an int. */
static inline bool VxWorks64TcpStream_FitsTheStack(size_t size)
{
    return size <= (size_t) INT_MAX;
}

/* A peer's FIN leaves the socket writable, so peek first: send would accept a
 * record that cannot arrive. */
static bool VxWorks64TcpStream_PeerIsStillThere(struct SolidSyslogVxWorks64TcpStream* self)
{
    char discard = 0;
    int peeked = recv(self->Fd, &discard, (int) sizeof(discard), MSG_PEEK | MSG_DONTWAIT);
    int peekErrno = (peeked == ERROR) ? errno : 0;
    return (peeked > 0) || ((peeked == ERROR) && VxWorks64TcpStream_WouldBlock(peekErrno));
}

/* EWOULDBLOCK and EAGAIN are distinct values in VxWorks 6.4. */
static inline bool VxWorks64TcpStream_WouldBlock(int err)
{
    return (err == EWOULDBLOCK) || (err == EAGAIN);
}

/* A short write or any error means the record did not go whole. */
static inline bool VxWorks64TcpStream_WroteAllBytes(int sent, size_t expected)
{
    return (sent >= 0) && ((size_t) sent == expected);
}

static SolidSyslogSsize VxWorks64TcpStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size)
{
    struct SolidSyslogVxWorks64TcpStream* self = VxWorks64TcpStream_SelfFromBase(base);
    int received = recv(self->Fd, (char*) buffer, VxWorks64TcpStream_ClampToTheStack(size), MSG_DONTWAIT);
    int recvErrno = (received == ERROR) ? errno : 0;
    SolidSyslogSsize result = -1;
    if (received > 0)
    {
        result = (SolidSyslogSsize) received;
    }
    else if ((received == ERROR) && VxWorks64TcpStream_WouldBlock(recvErrno))
    {
        result = 0;
    }
    else
    {
        /* The peer closed, or the connection failed. Close before reporting
         * it, as the Stream contract asks. */
        VxWorks64TcpStream_CloseSocket(self);
    }
    return result;
}

/* The stack takes a buffer's length as an int. */
static inline int VxWorks64TcpStream_ClampToTheStack(size_t size)
{
    return VxWorks64TcpStream_FitsTheStack(size) ? (int) size : INT_MAX;
}

static void VxWorks64TcpStream_Close(struct SolidSyslogStream* base)
{
    VxWorks64TcpStream_CloseSocket(VxWorks64TcpStream_SelfFromBase(base));
}

/* No runtime-tunable configuration; the destination is versioned by the
 * sender's endpoint. */
static uint32_t VxWorks64TcpStream_Version(struct SolidSyslogStream* base)
{
    (void) base;
    return 0U;
}
