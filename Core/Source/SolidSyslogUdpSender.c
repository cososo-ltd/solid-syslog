#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "SolidSyslogEndpoint.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogUdpPayload.h"
#include "SolidSyslogUdpSender.h"
#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogAddress.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogTransport.h"

struct SolidSyslogUdpSender
{
    struct SolidSyslogSender          base;
    struct SolidSyslogUdpSenderConfig config;
    SolidSyslogAddressStorage         addrStorage;
    bool                              connected;
    uint32_t                          lastEndpointVersion;
};

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
static void                                      NilEndpoint(struct SolidSyslogEndpoint* endpoint);
static uint32_t                                  NilEndpointVersion(void);

static const struct SolidSyslogUdpSender DEFAULT_INSTANCE = {.config = {.endpoint = NilEndpoint, .endpointVersion = NilEndpointVersion}};
static struct SolidSyslogUdpSender       instance;

struct SolidSyslogSender* SolidSyslogUdpSender_Create(const struct SolidSyslogUdpSenderConfig* config)
{
    instance                 = DEFAULT_INSTANCE;
    instance.config.resolver = config->resolver;
    instance.config.datagram = config->datagram;
    if (config->endpoint != NULL)
    {
        instance.config.endpoint = config->endpoint;
    }
    if (config->endpointVersion != NULL)
    {
        instance.config.endpointVersion = config->endpointVersion;
    }
    instance.base.Send       = Send;
    instance.base.Disconnect = Disconnect;
    return &instance.base;
}

void SolidSyslogUdpSender_Destroy(void)
{
    Disconnect(&instance.base);
    instance = DEFAULT_INSTANCE;
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

static void NilEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    SolidSyslogFormatter_BoundedString(endpoint->host, "", 0);
    endpoint->port = 0;
}

static uint32_t NilEndpointVersion(void)
{
    return 0;
}
