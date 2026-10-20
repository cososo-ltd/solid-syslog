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
#include "SolidSyslogDatagramCategories.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogPlusTcpAddressPrivate.h"
#include "SolidSyslogPlusTcpDatagramErrors.h"
#include "SolidSyslogPlusTcpDatagramPrivate.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogUdpPayload.h"
#include "task.h"

const struct SolidSyslogErrorSource SolidSyslogPlusTcpDatagramErrorSource = {
    "PlusTcpDatagram",
    SolidSyslogDatagramErrors_Name
};

enum
{
    PLUSTCP_DATAGRAM_MILLISECONDS_PER_SECOND = 1000U
};

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
static inline bool PlusTcpDatagram_NextHopResolves(struct SolidSyslogPlusTcpDatagram* self, uint32_t ip);
static inline eARPLookupResult_t PlusTcpDatagram_LookUpNextHop(uint32_t ip, uint32_t* nextHop);
static inline TickType_t PlusTcpDatagram_ResolveWaitTicks(void);
static inline void PlusTcpDatagram_ReportUnresolved(eARPLookupResult_t lookup);

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
    self->ResolveFailing = false;
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
        if (PlusTcpDatagram_NextHopResolves(self, dest->sin_address.ulIP_IPv4))
        {
            int32_t sent = FreeRTOS_sendto(self->Socket, buffer, size, 0, dest, sizeof(*dest));
            if (sent > 0)
            {
                result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT;
            }
        }
    }
    return result;
}

/* The stack drops a datagram whose next hop it is still resolving, so a
 * record is handed over only once the next hop is resolved. Each send that
 * finds it unresolved sends one request, which the stack does not repeat, so
 * a request lost during the wait goes again with the next send. While
 * resolution keeps failing the send asks but does not wait, so an unreachable
 * next hop costs one wait, not one per record. The stack's own
 * xARPWaitResolution is not used: it sleeps 250 ms between checks and does
 * not wait at all for an entry whose reply is pending. */
static inline bool PlusTcpDatagram_NextHopResolves(struct SolidSyslogPlusTcpDatagram* self, uint32_t ip)
{
    uint32_t nextHop = 0U;
    eARPLookupResult_t lookup = PlusTcpDatagram_LookUpNextHop(ip, &nextHop);
    if (lookup != eARPCacheHit)
    {
        FreeRTOS_OutputARPRequest(nextHop);
    }
    TickType_t ticksToWait = self->ResolveFailing ? 0U : PlusTcpDatagram_ResolveWaitTicks();
    for (TickType_t tick = 0U; (tick < ticksToWait) && (lookup != eARPCacheHit); tick++)
    {
        vTaskDelay(1U);
        lookup = PlusTcpDatagram_LookUpNextHop(ip, &nextHop);
    }
    bool resolved = (lookup == eARPCacheHit);
    if (!resolved && !self->ResolveFailing)
    {
        PlusTcpDatagram_ReportUnresolved(lookup);
    }
    self->ResolveFailing = !resolved;
    return resolved;
}

/* The stack names the next hop: the destination when it is on the subnet,
 * otherwise the gateway. */
static inline eARPLookupResult_t PlusTcpDatagram_LookUpNextHop(uint32_t ip, uint32_t* nextHop)
{
    MACAddress_t linkAddress;
    NetworkEndPoint_t* endPoint = NULL;
    *nextHop = ip;
    return eARPGetCacheEntry(nextHop, &linkAddress, &endPoint);
}

/* Rounded up, so a wait shorter than a tick is a tick rather than none. */
static inline TickType_t PlusTcpDatagram_ResolveWaitTicks(void)
{
    return (TickType_t) ((((uint32_t) SOLIDSYSLOG_DATAGRAM_RESOLVE_WAIT_MS * (uint32_t) configTICK_RATE_HZ) +
                          (PLUSTCP_DATAGRAM_MILLISECONDS_PER_SECOND - 1U)) /
                         PLUSTCP_DATAGRAM_MILLISECONDS_PER_SECOND);
}

/* The lookup's own answer follows as the native code: a miss, or
 * eCantSendPacket for an entry whose reply is still pending, or a next hop the
 * stack cannot send to at all, such as an off-subnet destination with no
 * gateway configured. */
static inline void PlusTcpDatagram_ReportUnresolved(eARPLookupResult_t lookup)
{
    PlusTcpDatagram_Report(
        SOLIDSYSLOG_DATAGRAM_NEXT_HOP_UNRESOLVED_SEVERITY,
        SOLIDSYSLOG_CAT_DATAGRAM_NEXT_HOP_UNRESOLVED,
        SOLIDSYSLOG_DATAGRAM_ERROR_NEXT_HOP_UNRESOLVED
    );
    SolidSyslog_Error(
        SOLIDSYSLOG_DATAGRAM_NEXT_HOP_UNRESOLVED_SEVERITY,
        &SolidSyslogPlusTcpDatagramErrorSource,
        SOLIDSYSLOG_CAT_NATIVE_ERROR,
        (int32_t) lookup
    );
}

static size_t PlusTcpDatagram_MaxPayload(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogPlusTcpDatagram* self = PlusTcpDatagram_SelfFromBase(base);
    return SolidSyslogUdpPayload_Stated(self->Config.GetPayloadLimit, self->Config.PayloadLimitContext);
}

// NOLINTEND(performance-no-int-to-ptr)
