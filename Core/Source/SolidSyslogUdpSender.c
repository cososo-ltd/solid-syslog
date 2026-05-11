#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "SolidSyslogAddress.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogEndpoint.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogTransport.h"
#include "SolidSyslogUdpPayload.h"
#include "SolidSyslogUdpSender.h"

struct SolidSyslogUdpSender
{
    struct SolidSyslogSender          base;
    struct SolidSyslogUdpSenderConfig config;
    SolidSyslogAddressStorage         addrStorage;
    bool                              connected;
    uint32_t                          lastEndpointVersion;
};

/* Forward declarations for the Nil "class" defined at the bottom of the file.
 * The Nil objects act as default collaborators that make the singleton
 * crash-safe pre-Create and after a Create that left required slots NULL;
 * the file-static instance below and the NilInstance template both reference
 * these by address. */
static struct SolidSyslogResolver NilResolver;
static struct SolidSyslogDatagram NilDatagram;
static void                       NilEndpoint(struct SolidSyslogEndpoint* endpoint);
static uint32_t                   NilEndpointVersion(void);

static struct SolidSyslogSender*                 InstallConfig(const struct SolidSyslogUdpSenderConfig* config);
static void                                      InstallResolver(struct SolidSyslogResolver* configured);
static void                                      InstallDatagram(struct SolidSyslogDatagram* configured);
static void                                      InstallEndpoint(SolidSyslogEndpointFunction configured);
static void                                      InstallEndpointVersion(SolidSyslogEndpointVersionFunction configured);
static bool                                      Send(struct SolidSyslogSender* self, const void* buffer, size_t size);
static inline bool                               Reconcile(struct SolidSyslogUdpSender* udp);
static inline void                               DisconnectIfStale(struct SolidSyslogUdpSender* udp);
static inline bool                               EnsureConnected(struct SolidSyslogUdpSender* udp);
static inline bool                               Connected(struct SolidSyslogUdpSender* udp);
static bool                                      Connect(struct SolidSyslogUdpSender* udp);
static void                                      Disconnect(struct SolidSyslogSender* self);
static inline bool                               OpenSocket(struct SolidSyslogUdpSender* udp);
static bool                                      ResolveDestination(struct SolidSyslogUdpSender* udp, const char* host, uint16_t port);
static inline struct SolidSyslogAddress*         Address(struct SolidSyslogUdpSender* udp);
static inline void                               CloseSocket(struct SolidSyslogUdpSender* udp);
static inline bool                               TransmitDatagram(struct SolidSyslogUdpSender* udp, const void* buffer, size_t size);
static inline enum SolidSyslogDatagramSendResult RetryAfterOversize(struct SolidSyslogUdpSender* udp, const void* buffer, size_t size);

/* Template used by _Create and _Destroy to reset every slot to its nil.
 * C99 forbids initialising one file-static from another, so the same
 * literal appears once below for the live instance and once here; both
 * sites stay in sync trivially because the values are nil-object
 * addresses and well-known no-op function pointers. */
static const struct SolidSyslogUdpSender NilInstance = {
    .base   = {.Send = Send, .Disconnect = Disconnect},
    .config = {.resolver = &NilResolver, .datagram = &NilDatagram, .endpoint = NilEndpoint, .endpointVersion = NilEndpointVersion},
};

static struct SolidSyslogUdpSender instance = {
    .base   = {.Send = Send, .Disconnect = Disconnect},
    .config = {.resolver = &NilResolver, .datagram = &NilDatagram, .endpoint = NilEndpoint, .endpointVersion = NilEndpointVersion},
};

static bool nilResolverReportArmed = true;
static bool nilDatagramReportArmed = true;
static bool instanceInitialised;

struct SolidSyslogSender* SolidSyslogUdpSender_Create(const struct SolidSyslogUdpSenderConfig* config)
{
    struct SolidSyslogSender* result = NULL;

    if (instanceInitialised)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_UDP_CREATE_ALREADY_INITIALISED);
        result = &instance.base;
    }
    else if (config == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_UDP_CREATE_NULL_CONFIG);
    }
    else
    {
        result              = InstallConfig(config);
        instanceInitialised = true;
    }

    return result;
}

static struct SolidSyslogSender* InstallConfig(const struct SolidSyslogUdpSenderConfig* config)
{
    instance = NilInstance;
    InstallResolver(config->resolver);
    InstallDatagram(config->datagram);
    InstallEndpoint(config->endpoint);
    InstallEndpointVersion(config->endpointVersion);
    return &instance.base;
}

static void InstallResolver(struct SolidSyslogResolver* configured)
{
    if (configured == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_UDP_CREATE_NULL_RESOLVER);
    }
    else
    {
        instance.config.resolver = configured;
    }
}

static void InstallDatagram(struct SolidSyslogDatagram* configured)
{
    if (configured == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_UDP_CREATE_NULL_DATAGRAM);
    }
    else
    {
        instance.config.datagram = configured;
    }
}

static void InstallEndpoint(SolidSyslogEndpointFunction configured)
{
    if (configured != NULL)
    {
        instance.config.endpoint = configured;
    }
}

static void InstallEndpointVersion(SolidSyslogEndpointVersionFunction configured)
{
    if (configured != NULL)
    {
        instance.config.endpointVersion = configured;
    }
}

void SolidSyslogUdpSender_Destroy(void)
{
    Disconnect(&instance.base);
    instance               = NilInstance;
    nilResolverReportArmed = true;
    nilDatagramReportArmed = true;
    instanceInitialised    = false;
}

static bool Send(struct SolidSyslogSender* self, const void* buffer, size_t size)
{
    struct SolidSyslogUdpSender* udp = (struct SolidSyslogUdpSender*) self;
    return Reconcile(udp) && TransmitDatagram(udp, buffer, size);
}

static inline bool Reconcile(struct SolidSyslogUdpSender* udp)
{
    DisconnectIfStale(udp);
    return EnsureConnected(udp);
}

static inline void DisconnectIfStale(struct SolidSyslogUdpSender* udp)
{
    uint32_t version = udp->config.endpointVersion();

    if (version != udp->lastEndpointVersion)
    {
        Disconnect(&udp->base);
        udp->lastEndpointVersion = version;
    }
}

static inline bool EnsureConnected(struct SolidSyslogUdpSender* udp)
{
    return Connected(udp) || Connect(udp);
}

static inline bool Connected(struct SolidSyslogUdpSender* udp)
{
    return udp->connected;
}

static bool Connect(struct SolidSyslogUdpSender* udp)
{
    SolidSyslogFormatterStorage  hostStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_HOST_SIZE)];
    struct SolidSyslogFormatter* hostFormatter = SolidSyslogFormatter_Create(hostStorage, SOLIDSYSLOG_MAX_HOST_SIZE);
    struct SolidSyslogEndpoint   endpoint      = {.host = hostFormatter, .port = 0};

    udp->config.endpoint(&endpoint);

    udp->connected = OpenSocket(udp) && ResolveDestination(udp, SolidSyslogFormatter_AsFormattedBuffer(hostFormatter), endpoint.port);

    if (!Connected(udp))
    {
        CloseSocket(udp);
    }

    return Connected(udp);
}

static void Disconnect(struct SolidSyslogSender* self)
{
    struct SolidSyslogUdpSender* udp = (struct SolidSyslogUdpSender*) self;

    if (Connected(udp))
    {
        CloseSocket(udp);
    }
}

static inline bool OpenSocket(struct SolidSyslogUdpSender* udp)
{
    return SolidSyslogDatagram_Open(udp->config.datagram);
}

static bool ResolveDestination(struct SolidSyslogUdpSender* udp, const char* host, uint16_t port)
{
    return SolidSyslogResolver_Resolve(udp->config.resolver, SOLIDSYSLOG_TRANSPORT_UDP, host, port, Address(udp));
}

static inline struct SolidSyslogAddress* Address(struct SolidSyslogUdpSender* udp)
{
    return SolidSyslogAddress_FromStorage(&udp->addrStorage);
}

static inline void CloseSocket(struct SolidSyslogUdpSender* udp)
{
    SolidSyslogDatagram_Close(udp->config.datagram);
    udp->connected = false;
}

/* Connected UDP fail/swallow contract:
 *   First Send fails non-OVERSIZE   → return false (transient — caller retries).
 *   First Send returns OVERSIZE     → trim and retry; propagate retry's verdict.
 *   Retry trimmed length is 0       → message can't physically fit the path;
 *                                     swallow and return true so the buffered
 *                                     algorithm doesn't loop on an undeliverable.
 *   Retry second Send succeeds      → return true.
 *   Retry second Send fails OVERSIZE→ kernel disagrees with its own reported
 *                                     MaxPayload — should be impossible. Swallow
 *                                     to avoid a permanent retry loop further up.
 *   Retry second Send fails other   → return false (transient).
 */
static inline bool TransmitDatagram(struct SolidSyslogUdpSender* udp, const void* buffer, size_t size)
{
    enum SolidSyslogDatagramSendResult result = SolidSyslogDatagram_SendTo(udp->config.datagram, buffer, size, Address(udp));
    if (result == SOLIDSYSLOG_DATAGRAM_OVERSIZE)
    {
        result = RetryAfterOversize(udp, buffer, size);
    }
    return result == SOLIDSYSLOG_DATAGRAM_SENT;
}

static inline enum SolidSyslogDatagramSendResult RetryAfterOversize(struct SolidSyslogUdpSender* udp, const void* buffer, size_t size)
{
    size_t                             maxPayload = SolidSyslogDatagram_MaxPayload(udp->config.datagram);
    size_t                             clipLimit  = (size < maxPayload) ? size : maxPayload;
    size_t                             trimmed    = SolidSyslogUdpPayload_TrimToCodepointBoundary((const uint8_t*) buffer, clipLimit);
    enum SolidSyslogDatagramSendResult result     = SOLIDSYSLOG_DATAGRAM_SENT;
    if (trimmed > 0)
    {
        result = SolidSyslogDatagram_SendTo(udp->config.datagram, buffer, trimmed, Address(udp));
        if (result == SOLIDSYSLOG_DATAGRAM_OVERSIZE)
        {
            result = SOLIDSYSLOG_DATAGRAM_SENT;
        }
    }
    return result;
}

/* ============================================================================
 * Nil collaborators
 *
 * Private no-op vtables and function-pointer defaults occupying every config
 * slot at file load and after _Destroy, plus any required slot the integrator
 * left NULL in their config. Vtable dispatch from Send never NULL-checks —
 * the nils make every collaborator pointer safe to call.
 *
 * NilResolver and NilDatagram each report one error via SolidSyslog_Error on
 * first dispatch, then silently consume; _Destroy re-arms the flags so a
 * Destroy/Create cycle produces a fresh warning. The endpoint and endpoint-
 * version nils are silent — supplying neither is a valid integrator choice
 * (matches NoEndpointConfiguredSendsToPortZero coverage).
 * ============================================================================ */

static bool NilResolverResolve(struct SolidSyslogResolver* self, enum SolidSyslogTransport transport, const char* host, uint16_t port,
                               struct SolidSyslogAddress* address)
{
    (void) self;
    (void) transport;
    (void) host;
    (void) port;
    (void) address;
    if (nilResolverReportArmed)
    {
        nilResolverReportArmed = false;
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_UDP_NIL_RESOLVER_USED);
    }
    return false;
}

static struct SolidSyslogResolver NilResolver = {.Resolve = NilResolverResolve};

static bool NilDatagramOpen(struct SolidSyslogDatagram* self)
{
    (void) self;
    if (nilDatagramReportArmed)
    {
        nilDatagramReportArmed = false;
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_UDP_NIL_DATAGRAM_USED);
    }
    return false;
}

static enum SolidSyslogDatagramSendResult NilDatagramSendTo(struct SolidSyslogDatagram* self, const void* buffer, size_t size,
                                                            const struct SolidSyslogAddress* address)
{
    (void) self;
    (void) buffer;
    (void) size;
    (void) address;
    return SOLIDSYSLOG_DATAGRAM_FAILED;
}

static size_t NilDatagramMaxPayload(struct SolidSyslogDatagram* self)
{
    (void) self;
    return 0;
}

static void NilDatagramClose(struct SolidSyslogDatagram* self)
{
    (void) self;
}

static struct SolidSyslogDatagram NilDatagram = {
    .Open       = NilDatagramOpen,
    .SendTo     = NilDatagramSendTo,
    .MaxPayload = NilDatagramMaxPayload,
    .Close      = NilDatagramClose,
};

static void NilEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    SolidSyslogFormatter_BoundedString(endpoint->host, "", 0);
    endpoint->port = 0;
}

static uint32_t NilEndpointVersion(void)
{
    return 0;
}
