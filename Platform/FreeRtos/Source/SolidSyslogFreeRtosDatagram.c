// NOLINTBEGIN(performance-no-int-to-ptr) -- FREERTOS_INVALID_SOCKET is ((Socket_t)~0U) from FreeRTOS-Plus-TCP; the int-to-ptr cast is intrinsic to the upstream
// sentinel and unavoidable.

#include "SolidSyslogFreeRtosDatagram.h"

#include "FreeRTOS.h"
#include "FreeRTOS_ARP.h"
#include "FreeRTOS_Sockets.h"
#include "task.h"

#include "SolidSyslogAddressInternal.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogMacros.h"
#include "SolidSyslogUdpPayload.h"

typedef struct SolidSyslogFreeRtosDatagram FreeRtosDatagram;

struct SolidSyslogFreeRtosDatagram
{
    struct SolidSyslogDatagram base;
    Socket_t socket;
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(FreeRtosDatagram) <= SOLIDSYSLOG_FREERTOSDATAGRAM_SIZE,
    "SOLIDSYSLOG_FREERTOSDATAGRAM_SIZE is too small for SolidSyslogFreeRtosDatagram layout"
);

/* Time the calling task yields after issuing an ARP probe so the IP task can
 * receive the reply and populate the cache before we attempt FreeRTOS_sendto.
 * 50 ms is generous against typical sub-millisecond LAN ARP RTT but short
 * enough that the first send latency stays tolerable. If the reply hasn't
 * arrived in time the sendto is allowed to fail or be dropped — UDP semantics. */
static const TickType_t ARP_RESOLUTION_WAIT_TICKS = pdMS_TO_TICKS(50);

static bool FreeRtosDatagram_Open(struct SolidSyslogDatagram* self);
static enum SolidSyslogDatagramSendResult FreeRtosDatagram_SendTo(
    struct SolidSyslogDatagram* self,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
);
static size_t FreeRtosDatagram_MaxPayload(struct SolidSyslogDatagram* self);
static void FreeRtosDatagram_Close(struct SolidSyslogDatagram* self);
static inline FreeRtosDatagram* FreeRtosDatagram_From(struct SolidSyslogDatagram* self);
static inline bool FreeRtosDatagram_IsOpen(const FreeRtosDatagram* datagram);
static inline void FreeRtosDatagram_PrimeArpIfMissing(uint32_t ip);

static const FreeRtosDatagram DEFAULT_INSTANCE = {
    {FreeRtosDatagram_Open, FreeRtosDatagram_SendTo, FreeRtosDatagram_MaxPayload, FreeRtosDatagram_Close},
    FREERTOS_INVALID_SOCKET,
};

static const FreeRtosDatagram DESTROYED_INSTANCE = {
    {NULL, NULL, NULL, NULL},
    FREERTOS_INVALID_SOCKET,
};

struct SolidSyslogDatagram* SolidSyslogFreeRtosDatagram_Create(SolidSyslogFreeRtosDatagramStorage* storage)
{
    FreeRtosDatagram* datagram = (FreeRtosDatagram*) storage;
    *datagram = DEFAULT_INSTANCE;
    return &datagram->base;
}

void SolidSyslogFreeRtosDatagram_Destroy(struct SolidSyslogDatagram* datagram)
{
    FreeRtosDatagram* self = FreeRtosDatagram_From(datagram);
    FreeRtosDatagram_Close(datagram);
    *self = DESTROYED_INSTANCE;
}

static inline FreeRtosDatagram* FreeRtosDatagram_From(struct SolidSyslogDatagram* self)
{
    return (FreeRtosDatagram*) self;
}

static bool FreeRtosDatagram_Open(struct SolidSyslogDatagram* self)
{
    FreeRtosDatagram* datagram = FreeRtosDatagram_From(self);
    if (!FreeRtosDatagram_IsOpen(datagram))
    {
        datagram->socket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP);
    }
    return FreeRtosDatagram_IsOpen(datagram);
}

static enum SolidSyslogDatagramSendResult FreeRtosDatagram_SendTo(
    struct SolidSyslogDatagram* self,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
)
{
    FreeRtosDatagram* datagram = FreeRtosDatagram_From(self);
    enum SolidSyslogDatagramSendResult result = SolidSyslogDatagramSendResult_Failed;
    if (FreeRtosDatagram_IsOpen(datagram))
    {
        const struct freertos_sockaddr* dest = SolidSyslogAddress_AsConstFreertosSockaddr(addr);
        FreeRtosDatagram_PrimeArpIfMissing(dest->sin_address.ulIP_IPv4);
        int32_t sent = FreeRTOS_sendto(datagram->socket, buffer, size, 0, dest, sizeof(*dest));
        if (sent > 0)
        {
            result = SolidSyslogDatagramSendResult_Sent;
        }
    }
    return result;
}

/* FreeRTOS-Plus-TCP does not queue datagrams while ARP resolves: a sendto to
 * an unresolved peer drops at the IP layer. Linux/Windows kernels mask this
 * with internal ARP queuing; FreeRTOS does not. So on cache miss we issue a
 * probe and yield once for the reply to land. If the reply hasn't arrived in
 * time the sendto is allowed to fail or be dropped — UDP is best-effort and
 * retry belongs in the store-and-forward layer above, not here. */
static inline void FreeRtosDatagram_PrimeArpIfMissing(uint32_t ip)
{
    if (xIsIPInARPCache(ip) == pdFALSE)
    {
        FreeRTOS_OutputARPRequest(ip);
        vTaskDelay(ARP_RESOLUTION_WAIT_TICKS);
    }
}

static inline bool FreeRtosDatagram_IsOpen(const FreeRtosDatagram* datagram)
{
    return datagram->socket != FREERTOS_INVALID_SOCKET;
}

static size_t FreeRtosDatagram_MaxPayload(struct SolidSyslogDatagram* self)
{
    (void) self;
    return SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD;
}

static void FreeRtosDatagram_Close(struct SolidSyslogDatagram* self)
{
    FreeRtosDatagram* datagram = FreeRtosDatagram_From(self);
    if (FreeRtosDatagram_IsOpen(datagram))
    {
        (void) FreeRTOS_closesocket(datagram->socket);
        datagram->socket = FREERTOS_INVALID_SOCKET;
    }
}

// NOLINTEND(performance-no-int-to-ptr)
