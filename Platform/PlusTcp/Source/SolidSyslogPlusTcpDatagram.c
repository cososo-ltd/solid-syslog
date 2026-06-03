// NOLINTBEGIN(performance-no-int-to-ptr) -- FREERTOS_INVALID_SOCKET is ((Socket_t)~0U) from FreeRTOS-Plus-TCP; the int-to-ptr cast is intrinsic to the upstream
// sentinel and unavoidable.

#include "SolidSyslogPlusTcpDatagram.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "FreeRTOS_ARP.h"
#include "FreeRTOS_Sockets.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogPlusTcpAddressPrivate.h"
#include "SolidSyslogPlusTcpDatagramErrors.h"
#include "SolidSyslogPlusTcpDatagramPrivate.h"
#include "SolidSyslogUdpPayload.h"
#include "task.h"

const struct SolidSyslogErrorSource PlusTcpDatagramErrorSource = {"PlusTcpDatagram"};

struct SolidSyslogAddress;

static bool PlusTcpDatagram_Open(struct SolidSyslogDatagram* base);
static enum SolidSyslogDatagramSendResult PlusTcpDatagram_SendTo(
    struct SolidSyslogDatagram* base,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
);
static size_t PlusTcpDatagram_MaxPayload(struct SolidSyslogDatagram* base);
static void PlusTcpDatagram_Close(struct SolidSyslogDatagram* base);

static inline struct SolidSyslogPlusTcpDatagram* PlusTcpDatagram_SelfFromBase(struct SolidSyslogDatagram* base);
static inline bool PlusTcpDatagram_IsOpen(const struct SolidSyslogPlusTcpDatagram* self);
static inline void PlusTcpDatagram_PrimeArpIfMissing(uint32_t ip);

void PlusTcpDatagram_Initialise(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogPlusTcpDatagram* self = PlusTcpDatagram_SelfFromBase(base);
    self->Base.Open = PlusTcpDatagram_Open;
    self->Base.SendTo = PlusTcpDatagram_SendTo;
    self->Base.MaxPayload = PlusTcpDatagram_MaxPayload;
    self->Base.Close = PlusTcpDatagram_Close;
    self->Socket = FREERTOS_INVALID_SOCKET;
}

static inline struct SolidSyslogPlusTcpDatagram* PlusTcpDatagram_SelfFromBase(struct SolidSyslogDatagram* base)
{
    return (struct SolidSyslogPlusTcpDatagram*) base;
}

void PlusTcpDatagram_Cleanup(struct SolidSyslogDatagram* base)
{
    PlusTcpDatagram_Close(base);
    /* Overwrite the abstract base with the shared NullDatagram vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullDatagram_Get();
}

static void PlusTcpDatagram_Close(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogPlusTcpDatagram* self = PlusTcpDatagram_SelfFromBase(base);
    if (PlusTcpDatagram_IsOpen(self))
    {
        (void) FreeRTOS_closesocket(self->Socket);
        self->Socket = FREERTOS_INVALID_SOCKET;
    }
}

static bool PlusTcpDatagram_Open(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogPlusTcpDatagram* self = PlusTcpDatagram_SelfFromBase(base);
    if (!PlusTcpDatagram_IsOpen(self))
    {
        self->Socket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP);
    }
    return PlusTcpDatagram_IsOpen(self);
}

static inline bool PlusTcpDatagram_IsOpen(const struct SolidSyslogPlusTcpDatagram* self)
{
    return self->Socket != FREERTOS_INVALID_SOCKET;
}

static enum SolidSyslogDatagramSendResult PlusTcpDatagram_SendTo(
    struct SolidSyslogDatagram* base,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
)
{
    struct SolidSyslogPlusTcpDatagram* self = PlusTcpDatagram_SelfFromBase(base);
    enum SolidSyslogDatagramSendResult result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED;
    if (PlusTcpDatagram_IsOpen(self))
    {
        const struct freertos_sockaddr* dest = SolidSyslogPlusTcpAddress_AsConstFreertosSockaddr(addr);
        PlusTcpDatagram_PrimeArpIfMissing(dest->sin_address.ulIP_IPv4);
        int32_t sent = FreeRTOS_sendto(self->Socket, buffer, size, 0, dest, sizeof(*dest));
        if (sent > 0)
        {
            result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT;
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
static inline void PlusTcpDatagram_PrimeArpIfMissing(uint32_t ip)
{
    /* Time the calling task yields after issuing an ARP probe so the IP
     * task can receive the reply and populate the cache before we
     * attempt FreeRTOS_sendto. 50 ms is generous against typical
     * sub-millisecond LAN ARP RTT but short enough that the first send
     * latency stays tolerable. If the reply hasn't arrived in time the
     * sendto is allowed to fail or be dropped — UDP semantics. */
    static const TickType_t ARP_RESOLUTION_WAIT_TICKS = pdMS_TO_TICKS(50);

    if (xIsIPInARPCache(ip) == pdFALSE)
    {
        FreeRTOS_OutputARPRequest(ip);
        vTaskDelay(ARP_RESOLUTION_WAIT_TICKS);
    }
}

static size_t PlusTcpDatagram_MaxPayload(struct SolidSyslogDatagram* base)
{
    (void) base;
    return SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD;
}

// NOLINTEND(performance-no-int-to-ptr)
