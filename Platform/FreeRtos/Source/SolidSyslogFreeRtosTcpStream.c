// NOLINTBEGIN(performance-no-int-to-ptr) -- FREERTOS_INVALID_SOCKET is ((Socket_t)~0U) from FreeRTOS-Plus-TCP; the int-to-ptr cast is intrinsic to the upstream
// sentinel and unavoidable.

#include "SolidSyslogFreeRtosTcpStream.h"

#include "FreeRTOS.h"
#include "FreeRTOS_ARP.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "task.h"

#include "SolidSyslogAddressInternal.h"
#include "SolidSyslogMacros.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"

typedef struct SolidSyslogFreeRtosTcpStream FreeRtosTcpStream;

struct SolidSyslogFreeRtosTcpStream
{
    struct SolidSyslogStream Base;
    Socket_t Socket;
};

/* 200 ms is short enough that the Service task keeps draining predictably
 * during an outage, long enough for a healthy peer to ACK over slirp/LAN.
 * Both SO_SNDTIMEO and SO_RCVTIMEO are set before FreeRTOS_connect —
 * upstream gates connect on SO_RCVTIMEO, but we set both as belt-and-braces
 * against an upstream change. After connect both timeouts go back to 0 so
 * subsequent Send/Read follow the non-blocking single-call contract from
 * SolidSyslogStream. */
static const TickType_t CONNECT_TIMEOUT_TICKS = pdMS_TO_TICKS(200);
static const TickType_t NO_TIMEOUT_TICKS = 0;

/* Yield window for the IP task to receive an ARP reply and populate the
 * cache before we attempt FreeRTOS_connect. Mirrors the established
 * SolidSyslogFreeRtosDatagram pattern (see [[freertos-arp-first-packet]]) —
 * without this, a cold-start TCP connect fires SYN before ARP resolves, the
 * SYN is dropped at the IP layer, and the bounded 200 ms RCV-timeout
 * connect expires before the retransmit ARP-and-resend cycle completes. */
static const TickType_t ARP_RESOLUTION_WAIT_TICKS = pdMS_TO_TICKS(50);

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

static bool FreeRtosTcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);
static bool FreeRtosTcpStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size);
static SolidSyslogSsize FreeRtosTcpStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size);
static void FreeRtosTcpStream_Close(struct SolidSyslogStream* base);

static inline FreeRtosTcpStream* FreeRtosTcpStream_SelfFromStorage(SolidSyslogFreeRtosTcpStreamStorage* storage);
static inline FreeRtosTcpStream* FreeRtosTcpStream_SelfFromBase(struct SolidSyslogStream* base);
static inline bool FreeRtosTcpStream_IsOpen(const FreeRtosTcpStream* self);
static inline bool FreeRtosTcpStream_IsClosed(const FreeRtosTcpStream* self);
static void FreeRtosTcpStream_OpenSocket(FreeRtosTcpStream* self);
static void FreeRtosTcpStream_ConnectOrCloseOnFailure(FreeRtosTcpStream* self, const struct SolidSyslogAddress* addr);
static bool FreeRtosTcpStream_TryConnect(FreeRtosTcpStream* self, const struct SolidSyslogAddress* addr);
static void FreeRtosTcpStream_ClearTimeouts(Socket_t socket);
static void FreeRtosTcpStream_SetSendTimeout(Socket_t socket, TickType_t ticks);
static void FreeRtosTcpStream_SetRecvTimeout(Socket_t socket, TickType_t ticks);
static bool FreeRtosTcpStream_SendOrCloseOnFailure(FreeRtosTcpStream* self, const void* buffer, size_t size);
static bool FreeRtosTcpStream_TrySend(FreeRtosTcpStream* self, const void* buffer, size_t size);
static bool FreeRtosTcpStream_AllBytesSent(BaseType_t sentCount, size_t expected);
static SolidSyslogSsize FreeRtosTcpStream_ReceiveOrCloseOnFailure(FreeRtosTcpStream* self, void* buffer, size_t size);
static void FreeRtosTcpStream_CloseSocket(FreeRtosTcpStream* self);

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(FreeRtosTcpStream) <= SOLIDSYSLOG_FREERTOSTCPSTREAM_SIZE,
    "SOLIDSYSLOG_FREERTOSTCPSTREAM_SIZE is too small for SolidSyslogFreeRtosTcpStream layout"
);

static const FreeRtosTcpStream DEFAULT_INSTANCE = {
    {FreeRtosTcpStream_Open, FreeRtosTcpStream_Send, FreeRtosTcpStream_Read, FreeRtosTcpStream_Close},
    FREERTOS_INVALID_SOCKET,
};

static const FreeRtosTcpStream DESTROYED_INSTANCE = {
    {NULL, NULL, NULL, NULL},
    FREERTOS_INVALID_SOCKET,
};

struct SolidSyslogStream* SolidSyslogFreeRtosTcpStream_Create(SolidSyslogFreeRtosTcpStreamStorage* storage)
{
    FreeRtosTcpStream* self = FreeRtosTcpStream_SelfFromStorage(storage);
    *self = DEFAULT_INSTANCE;
    return &self->Base;
}

static inline FreeRtosTcpStream* FreeRtosTcpStream_SelfFromStorage(SolidSyslogFreeRtosTcpStreamStorage* storage)
{
    return (FreeRtosTcpStream*) storage;
}

void SolidSyslogFreeRtosTcpStream_Destroy(struct SolidSyslogStream* base)
{
    FreeRtosTcpStream* self = FreeRtosTcpStream_SelfFromBase(base);
    FreeRtosTcpStream_CloseSocket(self);
    *self = DESTROYED_INSTANCE;
}

static inline FreeRtosTcpStream* FreeRtosTcpStream_SelfFromBase(struct SolidSyslogStream* base)
{
    return (FreeRtosTcpStream*) base;
}

static bool FreeRtosTcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr)
{
    FreeRtosTcpStream* self = FreeRtosTcpStream_SelfFromBase(base);
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

static inline bool FreeRtosTcpStream_IsOpen(const FreeRtosTcpStream* self)
{
    return self->Socket != FREERTOS_INVALID_SOCKET;
}

static inline bool FreeRtosTcpStream_IsClosed(const FreeRtosTcpStream* self)
{
    return !FreeRtosTcpStream_IsOpen(self);
}

static void FreeRtosTcpStream_OpenSocket(FreeRtosTcpStream* self)
{
    self->Socket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
}

static void FreeRtosTcpStream_ConnectOrCloseOnFailure(FreeRtosTcpStream* self, const struct SolidSyslogAddress* addr)
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

static inline void FreeRtosTcpStream_PrimeArpIfMissing(uint32_t ip);

static bool FreeRtosTcpStream_TryConnect(FreeRtosTcpStream* self, const struct SolidSyslogAddress* addr)
{
    const struct freertos_sockaddr* dest = SolidSyslogAddress_AsConstFreertosSockaddr(addr);
    FreeRtosTcpStream_PrimeArpIfMissing(dest->sin_address.ulIP_IPv4);
    FreeRtosTcpStream_SetSendTimeout(self->Socket, CONNECT_TIMEOUT_TICKS);
    FreeRtosTcpStream_SetRecvTimeout(self->Socket, CONNECT_TIMEOUT_TICKS);
    return FreeRTOS_connect(self->Socket, dest, sizeof(*dest)) == 0;
}

/* On ARP cache miss issue a probe and yield once for the reply to land
 * before FreeRTOS_connect runs. Without this, the cold-start SYN is dropped
 * at the IP layer (FreeRTOS-Plus-TCP does not queue while ARP resolves) and
 * the bounded 200 ms connect timeout expires before the SYN-and-resend
 * cycle completes. Symmetric with SolidSyslogFreeRtosDatagram::SendTo. */
static inline void FreeRtosTcpStream_PrimeArpIfMissing(uint32_t ip)
{
    if (xIsIPInARPCache(ip) == pdFALSE)
    {
        FreeRTOS_OutputARPRequest(ip);
        vTaskDelay(ARP_RESOLUTION_WAIT_TICKS);
    }
}

static void FreeRtosTcpStream_ClearTimeouts(Socket_t socket)
{
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
    FreeRtosTcpStream* self = FreeRtosTcpStream_SelfFromBase(base);
    bool sent = false;
    if (FreeRtosTcpStream_IsOpen(self))
    {
        sent = FreeRtosTcpStream_SendOrCloseOnFailure(self, buffer, size);
    }
    return sent;
}

static bool FreeRtosTcpStream_SendOrCloseOnFailure(FreeRtosTcpStream* self, const void* buffer, size_t size)
{
    bool sent = FreeRtosTcpStream_TrySend(self, buffer, size);
    if (!sent)
    {
        FreeRtosTcpStream_CloseSocket(self);
    }
    return sent;
}

static bool FreeRtosTcpStream_TrySend(FreeRtosTcpStream* self, const void* buffer, size_t size)
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
    FreeRtosTcpStream* self = FreeRtosTcpStream_SelfFromBase(base);
    SolidSyslogSsize result = READ_FAILED;
    if (FreeRtosTcpStream_IsOpen(self))
    {
        result = FreeRtosTcpStream_ReceiveOrCloseOnFailure(self, buffer, size);
    }
    return result;
}

static SolidSyslogSsize FreeRtosTcpStream_ReceiveOrCloseOnFailure(FreeRtosTcpStream* self, void* buffer, size_t size)
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

static void FreeRtosTcpStream_CloseSocket(FreeRtosTcpStream* self)
{
    if (FreeRtosTcpStream_IsOpen(self))
    {
        (void) FreeRTOS_closesocket(self->Socket);
        self->Socket = FREERTOS_INVALID_SOCKET;
    }
}

// NOLINTEND(performance-no-int-to-ptr)
