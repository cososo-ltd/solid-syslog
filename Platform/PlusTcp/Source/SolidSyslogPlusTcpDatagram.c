/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

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
#include "SolidSyslogTunables.h"
#include "SolidSyslogUdpPayload.h"
#include "task.h"

const struct SolidSyslogErrorSource SolidSyslogPlusTcpDatagramErrorSource = {"PlusTcpDatagram"};

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
static inline void PlusTcpDatagram_WaitForTheNextHop(uint32_t ip);
static inline bool PlusTcpDatagram_LookUpNextHop(uint32_t ip, uint32_t* nextHop);
static inline TickType_t PlusTcpDatagram_ResolveWaitTicks(void);

void SolidSyslogPlusTcpDatagram_Initialise(
    struct SolidSyslogDatagram* base,
    const struct SolidSyslogPlusTcpDatagramConfig* config
)
{
    struct SolidSyslogPlusTcpDatagram* self = PlusTcpDatagram_SelfFromBase(base);
    self->Config.GetPayloadLimit = NULL;
    self->Config.PayloadLimitContext = NULL;
    if (config != NULL)
    {
        self->Config = *config;
    }
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

void SolidSyslogPlusTcpDatagram_Cleanup(struct SolidSyslogDatagram* base)
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
        PlusTcpDatagram_WaitForTheNextHop(dest->sin_address.ulIP_IPv4);
        int32_t sent = FreeRTOS_sendto(self->Socket, buffer, size, 0, dest, sizeof(*dest));
        if (sent > 0)
        {
            result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT;
        }
    }
    return result;
}

/* The stack drops a datagram whose next hop it is still resolving, so a
 * record is handed over only once the next hop is resolved. A miss sends one
 * request; the stack's ARP timer repeats it. */
static inline void PlusTcpDatagram_WaitForTheNextHop(uint32_t ip)
{
    uint32_t nextHop = 0U;
    bool resolved = PlusTcpDatagram_LookUpNextHop(ip, &nextHop);
    if (!resolved)
    {
        FreeRTOS_OutputARPRequest(nextHop);
    }
    for (TickType_t tick = 0U; (tick < PlusTcpDatagram_ResolveWaitTicks()) && !resolved; tick++)
    {
        vTaskDelay(1U);
        resolved = PlusTcpDatagram_LookUpNextHop(ip, &nextHop);
    }
}

/* The stack names the next hop: the destination when it is on the subnet,
 * otherwise the gateway. */
static inline bool PlusTcpDatagram_LookUpNextHop(uint32_t ip, uint32_t* nextHop)
{
    MACAddress_t linkAddress;
    NetworkEndPoint_t* endPoint = NULL;
    *nextHop = ip;
    return eARPGetCacheEntry(nextHop, &linkAddress, &endPoint) == eARPCacheHit;
}

/* Rounded up, so a wait shorter than a tick is a tick rather than none. */
static inline TickType_t PlusTcpDatagram_ResolveWaitTicks(void)
{
    TickType_t ticks = pdMS_TO_TICKS(SOLIDSYSLOG_DATAGRAM_RESOLVE_WAIT_MS);
    return (ticks == 0U) ? 1U : ticks;
}

static size_t PlusTcpDatagram_MaxPayload(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogPlusTcpDatagram* self = PlusTcpDatagram_SelfFromBase(base);
    return SolidSyslogUdpPayload_Stated(self->Config.GetPayloadLimit, self->Config.PayloadLimitContext);
}

// NOLINTEND(performance-no-int-to-ptr)
