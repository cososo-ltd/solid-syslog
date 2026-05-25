// NOLINTBEGIN(performance-no-int-to-ptr) -- FREERTOS_INVALID_SOCKET is ((Socket_t)~0U) from FreeRTOS-Plus-TCP; the int-to-ptr cast is intrinsic to the upstream
// sentinel and unavoidable.

#include "SolidSyslogFreeRtosTcpStream.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "FreeRTOS_ARP.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "task.h"

#include "SolidSyslogPlusTcpAddressPrivate.h"
#include "SolidSyslogFreeRtosTcpStreamPrivate.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTunables.h"

/* SolidSyslogStream_Read returns < 0 to signal EOF/error (socket closed
 * internally); -1 is the in-tree convention shared with Posix/Winsock. */
static const SolidSyslogSsize READ_FAILED = -1;

enum
{
    /* FreeRTOS-Plus-TCP's setsockopt ignores the level argument (no
     * SOL_SOCKET vs IPPROTO_TCP split; option codes are flat); pass 0 by
     * convention. */
    SETSOCKOPT_LEVEL_DEFAULT = 0,
    /* No MSG_PEEK / MSG_DONTWAIT / zero-copy — the timeouts cleared after
     * connect already give us the non-blocking single-call behaviour
     * SolidSyslogStream requires. */
    SEND_RECV_FLAGS_DEFAULT = 0
};

static uint32_t FreeRtosTcpStream_NullConnectTimeoutGetter(void* context);

static bool FreeRtosTcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);
static bool FreeRtosTcpStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size);
static SolidSyslogSsize FreeRtosTcpStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size);
static void FreeRtosTcpStream_Close(struct SolidSyslogStream* base);

static inline struct SolidSyslogFreeRtosTcpStream* FreeRtosTcpStream_SelfFromBase(struct SolidSyslogStream* base);
static inline bool FreeRtosTcpStream_ConfigProvidesGetter(const struct SolidSyslogFreeRtosTcpStreamConfig* config);
static inline bool FreeRtosTcpStream_IsOpen(const struct SolidSyslogFreeRtosTcpStream* self);
static inline bool FreeRtosTcpStream_IsClosed(const struct SolidSyslogFreeRtosTcpStream* self);
static void FreeRtosTcpStream_OpenSocket(struct SolidSyslogFreeRtosTcpStream* self);
static void FreeRtosTcpStream_ConnectOrCloseOnFailure(
    struct SolidSyslogFreeRtosTcpStream* self,
    const struct SolidSyslogAddress* addr
);
static bool FreeRtosTcpStream_TryConnect(
    struct SolidSyslogFreeRtosTcpStream* self,
    const struct SolidSyslogAddress* addr
);
static inline void FreeRtosTcpStream_PrimeArpIfMissing(uint32_t ip);
static void FreeRtosTcpStream_ClearTimeouts(Socket_t socket);
static void FreeRtosTcpStream_SetSendTimeout(Socket_t socket, TickType_t ticks);
static void FreeRtosTcpStream_SetRecvTimeout(Socket_t socket, TickType_t ticks);
static uint32_t FreeRtosTcpStream_ResolveConnectTimeoutMs(struct SolidSyslogFreeRtosTcpStream* self);
static bool FreeRtosTcpStream_SendOrCloseOnFailure(
    struct SolidSyslogFreeRtosTcpStream* self,
    const void* buffer,
    size_t size
);
static bool FreeRtosTcpStream_TrySend(struct SolidSyslogFreeRtosTcpStream* self, const void* buffer, size_t size);
static bool FreeRtosTcpStream_AllBytesSent(BaseType_t sentCount, size_t expected);
static SolidSyslogSsize FreeRtosTcpStream_ReceiveOrCloseOnFailure(
    struct SolidSyslogFreeRtosTcpStream* self,
    void* buffer,
    size_t size
);
static void FreeRtosTcpStream_CloseSocket(struct SolidSyslogFreeRtosTcpStream* self);

void FreeRtosTcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogFreeRtosTcpStreamConfig* config
)
{
    static const struct SolidSyslogFreeRtosTcpStream DefaultFreeRtosTcpStream = {
        .Base =
            {.Open = FreeRtosTcpStream_Open,
             .Send = FreeRtosTcpStream_Send,
             .Read = FreeRtosTcpStream_Read,
             .Close = FreeRtosTcpStream_Close},
        .Config = {.GetConnectTimeoutMs = FreeRtosTcpStream_NullConnectTimeoutGetter, .ConnectTimeoutContext = NULL},
        .Socket = FREERTOS_INVALID_SOCKET,
    };

    struct SolidSyslogFreeRtosTcpStream* self = FreeRtosTcpStream_SelfFromBase(base);
    *self = DefaultFreeRtosTcpStream;
    if (FreeRtosTcpStream_ConfigProvidesGetter(config) == true)
    {
        self->Config = *config;
    }
}

static inline bool FreeRtosTcpStream_ConfigProvidesGetter(const struct SolidSyslogFreeRtosTcpStreamConfig* config)
{
    return (config != NULL) && (config->GetConnectTimeoutMs != NULL);
}

/* Null Object substituted when the integrator does not install a getter —
 * returns the compile-time tunable so the bounded-wait path has a single
 * code path regardless of whether the integrator wired runtime tuning. */
static uint32_t FreeRtosTcpStream_NullConnectTimeoutGetter(void* context)
{
    (void) context;
    return (uint32_t) SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS;
}

static inline struct SolidSyslogFreeRtosTcpStream* FreeRtosTcpStream_SelfFromBase(struct SolidSyslogStream* base)
{
    return (struct SolidSyslogFreeRtosTcpStream*) base;
}

void FreeRtosTcpStream_Cleanup(struct SolidSyslogStream* base)
{
    struct SolidSyslogFreeRtosTcpStream* self = FreeRtosTcpStream_SelfFromBase(base);
    FreeRtosTcpStream_CloseSocket(self);
    /* Overwrite the abstract base with the shared NullStream vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullStream_Get();
}

static void FreeRtosTcpStream_CloseSocket(struct SolidSyslogFreeRtosTcpStream* self)
{
    if (FreeRtosTcpStream_IsOpen(self))
    {
        (void) FreeRTOS_closesocket(self->Socket);
        self->Socket = FREERTOS_INVALID_SOCKET;
    }
}

static bool FreeRtosTcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr)
{
    struct SolidSyslogFreeRtosTcpStream* self = FreeRtosTcpStream_SelfFromBase(base);
    if (FreeRtosTcpStream_IsClosed(self))
    {
        FreeRtosTcpStream_OpenSocket(self);
        if (FreeRtosTcpStream_IsOpen(self))
        {
            FreeRtosTcpStream_ConnectOrCloseOnFailure(self, addr);
        }
    }
    return FreeRtosTcpStream_IsOpen(self);
}

static inline bool FreeRtosTcpStream_IsOpen(const struct SolidSyslogFreeRtosTcpStream* self)
{
    return self->Socket != FREERTOS_INVALID_SOCKET;
}

static inline bool FreeRtosTcpStream_IsClosed(const struct SolidSyslogFreeRtosTcpStream* self)
{
    return !FreeRtosTcpStream_IsOpen(self);
}

static void FreeRtosTcpStream_OpenSocket(struct SolidSyslogFreeRtosTcpStream* self)
{
    self->Socket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
}

static void FreeRtosTcpStream_ConnectOrCloseOnFailure(
    struct SolidSyslogFreeRtosTcpStream* self,
    const struct SolidSyslogAddress* addr
)
{
    if (FreeRtosTcpStream_TryConnect(self, addr))
    {
        FreeRtosTcpStream_ClearTimeouts(self->Socket);
    }
    else
    {
        FreeRtosTcpStream_CloseSocket(self);
    }
}

static bool FreeRtosTcpStream_TryConnect(
    struct SolidSyslogFreeRtosTcpStream* self,
    const struct SolidSyslogAddress* addr
)
{
    /* Both SO_SNDTIMEO and SO_RCVTIMEO are set before FreeRTOS_connect —
     * upstream gates connect on SO_RCVTIMEO, but we set both as belt-and-
     * braces against an upstream change. After connect both timeouts go
     * back to 0 so subsequent Send/Read follow the non-blocking single-
     * call contract from SolidSyslogStream. The default
     * SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS = 200 is short enough that the
     * Service task keeps draining predictably during an outage, long
     * enough for a healthy peer to ACK over slirp/LAN. */
    const TickType_t connectTimeoutTicks = pdMS_TO_TICKS(FreeRtosTcpStream_ResolveConnectTimeoutMs(self));

    const struct freertos_sockaddr* dest = SolidSyslogPlusTcpAddress_AsConstFreertosSockaddr(addr);
    FreeRtosTcpStream_PrimeArpIfMissing(dest->sin_address.ulIP_IPv4);
    FreeRtosTcpStream_SetSendTimeout(self->Socket, connectTimeoutTicks);
    FreeRtosTcpStream_SetRecvTimeout(self->Socket, connectTimeoutTicks);
    return FreeRTOS_connect(self->Socket, dest, sizeof(*dest)) == 0;
}

/* Bridges the integrator-installed getter (or the Null Object substituted in
 * Initialise) to the bounded SO_*TIMEO deadline. Invoked on every connect
 * attempt so a runtime-tunable value takes effect on the next reconnect. */
static uint32_t FreeRtosTcpStream_ResolveConnectTimeoutMs(struct SolidSyslogFreeRtosTcpStream* self)
{
    return self->Config.GetConnectTimeoutMs(self->Config.ConnectTimeoutContext);
}

/* On ARP cache miss issue a probe and yield once for the reply to land
 * before FreeRTOS_connect runs. Without this, the cold-start SYN is dropped
 * at the IP layer (FreeRTOS-Plus-TCP does not queue while ARP resolves) and
 * the bounded 200 ms connect timeout expires before the SYN-and-resend
 * cycle completes. Symmetric with SolidSyslogPlusTcpDatagram::SendTo. */
static inline void FreeRtosTcpStream_PrimeArpIfMissing(uint32_t ip)
{
    /* Yield window for the IP task to receive an ARP reply and populate
     * the cache before we attempt FreeRTOS_connect. Mirrors the
     * established SolidSyslogPlusTcpDatagram pattern (see
     * [[freertos-arp-first-packet]]) — without this, a cold-start TCP
     * connect fires SYN before ARP resolves, the SYN is dropped at the
     * IP layer, and the bounded 200 ms RCV-timeout connect expires
     * before the retransmit ARP-and-resend cycle completes. */
    static const TickType_t ARP_RESOLUTION_WAIT_TICKS = pdMS_TO_TICKS(50);

    if (xIsIPInARPCache(ip) == pdFALSE)
    {
        FreeRTOS_OutputARPRequest(ip);
        vTaskDelay(ARP_RESOLUTION_WAIT_TICKS);
    }
}

static void FreeRtosTcpStream_ClearTimeouts(Socket_t socket)
{
    static const TickType_t NO_TIMEOUT_TICKS = 0;

    FreeRtosTcpStream_SetSendTimeout(socket, NO_TIMEOUT_TICKS);
    FreeRtosTcpStream_SetRecvTimeout(socket, NO_TIMEOUT_TICKS);
}

static void FreeRtosTcpStream_SetSendTimeout(Socket_t socket, TickType_t ticks)
{
    (void) FreeRTOS_setsockopt(socket, SETSOCKOPT_LEVEL_DEFAULT, FREERTOS_SO_SNDTIMEO, &ticks, sizeof(ticks));
}

static void FreeRtosTcpStream_SetRecvTimeout(Socket_t socket, TickType_t ticks)
{
    (void) FreeRTOS_setsockopt(socket, SETSOCKOPT_LEVEL_DEFAULT, FREERTOS_SO_RCVTIMEO, &ticks, sizeof(ticks));
}

static bool FreeRtosTcpStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size)
{
    struct SolidSyslogFreeRtosTcpStream* self = FreeRtosTcpStream_SelfFromBase(base);
    bool sent = false;
    if (FreeRtosTcpStream_IsOpen(self))
    {
        sent = FreeRtosTcpStream_SendOrCloseOnFailure(self, buffer, size);
    }
    return sent;
}

static bool FreeRtosTcpStream_SendOrCloseOnFailure(
    struct SolidSyslogFreeRtosTcpStream* self,
    const void* buffer,
    size_t size
)
{
    bool sent = FreeRtosTcpStream_TrySend(self, buffer, size);
    if (!sent)
    {
        FreeRtosTcpStream_CloseSocket(self);
    }
    return sent;
}

static bool FreeRtosTcpStream_TrySend(struct SolidSyslogFreeRtosTcpStream* self, const void* buffer, size_t size)
{
    BaseType_t sentCount = FreeRTOS_send(self->Socket, buffer, size, SEND_RECV_FLAGS_DEFAULT);
    return FreeRtosTcpStream_AllBytesSent(sentCount, size);
}

static bool FreeRtosTcpStream_AllBytesSent(BaseType_t sentCount, size_t expected)
{
    return (sentCount >= 0) && ((size_t) sentCount == expected);
}

/* FreeRTOS_recv with RCVTIMEO=0 returns 0 when no data is available (would
 * block); the Service thread treats that as "nothing to read right now".
 * Negative returns are real errors — close the socket and surface failure. */
static SolidSyslogSsize FreeRtosTcpStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size)
{
    struct SolidSyslogFreeRtosTcpStream* self = FreeRtosTcpStream_SelfFromBase(base);
    SolidSyslogSsize result = READ_FAILED;
    if (FreeRtosTcpStream_IsOpen(self))
    {
        result = FreeRtosTcpStream_ReceiveOrCloseOnFailure(self, buffer, size);
    }
    return result;
}

static SolidSyslogSsize FreeRtosTcpStream_ReceiveOrCloseOnFailure(
    struct SolidSyslogFreeRtosTcpStream* self,
    void* buffer,
    size_t size
)
{
    BaseType_t receivedCount = FreeRTOS_recv(self->Socket, buffer, size, SEND_RECV_FLAGS_DEFAULT);
    SolidSyslogSsize result = READ_FAILED;
    if (receivedCount >= 0)
    {
        result = (SolidSyslogSsize) receivedCount;
    }
    else
    {
        FreeRtosTcpStream_CloseSocket(self);
    }
    return result;
}

static void FreeRtosTcpStream_Close(struct SolidSyslogStream* base)
{
    FreeRtosTcpStream_CloseSocket(FreeRtosTcpStream_SelfFromBase(base));
}

// NOLINTEND(performance-no-int-to-ptr)
