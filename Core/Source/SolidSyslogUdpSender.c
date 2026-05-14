#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "SolidSyslogEndpoint.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
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

static bool                                      IsValidConfig(const struct SolidSyslogUdpSenderConfig* config);
static void                                      InstallConfig(const struct SolidSyslogUdpSenderConfig* config);
static bool                                      Send(struct SolidSyslogSender* self, const void* buffer, size_t size);
static void                                      Disconnect(struct SolidSyslogSender* self);
static inline bool                               Reconcile(struct SolidSyslogUdpSender* udp);
static inline void                               DisconnectIfStale(struct SolidSyslogUdpSender* udp);
static inline bool                               EnsureConnected(struct SolidSyslogUdpSender* udp);
static inline bool                               Connected(struct SolidSyslogUdpSender* udp);
static bool                                      Connect(struct SolidSyslogUdpSender* udp);
static inline uint16_t                           QueryEndpointPort(struct SolidSyslogUdpSender* udp, struct SolidSyslogFormatter* hostFormatter);
static inline bool                               OpenSocket(struct SolidSyslogUdpSender* udp);
static bool                                      ResolveDestination(struct SolidSyslogUdpSender* udp, const char* host, uint16_t port);
static inline struct SolidSyslogAddress*         Address(struct SolidSyslogUdpSender* udp);
static inline void                               CloseSocket(struct SolidSyslogUdpSender* udp);
static inline bool                               TransmitDatagram(struct SolidSyslogUdpSender* udp, const void* buffer, size_t size);
static inline enum SolidSyslogDatagramSendResult RetryAfterOversize(struct SolidSyslogUdpSender* udp, const void* buffer, size_t size);
static uint32_t                                  NilEndpointVersion(void);
static bool                                      NilUdpSenderSend(struct SolidSyslogSender* self, const void* buffer, size_t size);
static void                                      NilUdpSenderDisconnect(struct SolidSyslogSender* self);

static const struct SolidSyslogUdpSender DEFAULT_INSTANCE = {.config = {.endpointVersion = NilEndpointVersion}};
static struct SolidSyslogUdpSender       instance;
static struct SolidSyslogSender          NilUdpSender = {.Send = NilUdpSenderSend, .Disconnect = NilUdpSenderDisconnect};

struct SolidSyslogSender* SolidSyslogUdpSender_Create(const struct SolidSyslogUdpSenderConfig* config)
{
    struct SolidSyslogSender* result = &NilUdpSender;
    if (IsValidConfig(config))
    {
        InstallConfig(config);
        result = &instance.base;
    }
    return result;
}

static bool IsValidConfig(const struct SolidSyslogUdpSenderConfig* config)
{
    bool valid = false;
    if (config == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_CONFIG);
    }
    else if (config->resolver == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_RESOLVER);
    }
    else if (config->datagram == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_DATAGRAM);
    }
    else if (config->endpoint == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_ENDPOINT);
    }
    else
    {
        valid = true;
    }
    return valid;
}

static void InstallConfig(const struct SolidSyslogUdpSenderConfig* config)
{
    instance        = DEFAULT_INSTANCE;
    instance.config = *config;
    if (instance.config.endpointVersion == NULL)
    {
        instance.config.endpointVersion = NilEndpointVersion;
    }
    instance.base.Send       = Send;
    instance.base.Disconnect = Disconnect;
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

static void Disconnect(struct SolidSyslogSender* self)
{
    struct SolidSyslogUdpSender* udp = (struct SolidSyslogUdpSender*) self;

    if (Connected(udp))
    {
        CloseSocket(udp);
    }
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
    uint16_t                     port          = QueryEndpointPort(udp, hostFormatter);
    const char*                  host          = SolidSyslogFormatter_AsFormattedBuffer(hostFormatter);

    if (OpenSocket(udp) && ResolveDestination(udp, host, port))
    {
        udp->connected = true;
    }
    else
    {
        CloseSocket(udp);
    }
    return Connected(udp);
}

static inline uint16_t QueryEndpointPort(struct SolidSyslogUdpSender* udp, struct SolidSyslogFormatter* hostFormatter)
{
    struct SolidSyslogEndpoint endpoint = {.host = hostFormatter, .port = 0};
    udp->config.endpoint(&endpoint);
    return endpoint.port;
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
    size_t maxPayload = SolidSyslogDatagram_MaxPayload(udp->config.datagram);
    size_t clipLimit  = (size < maxPayload) ? size : maxPayload;
    size_t trimmed    = SolidSyslogUdpPayload_TrimToCodepointBoundary((const uint8_t*) buffer, clipLimit);
    /* Default SENT swallows trimmed == 0 (path can't carry the message) so the
     * Service algorithm doesn't loop on an undeliverable. */
    enum SolidSyslogDatagramSendResult result = SOLIDSYSLOG_DATAGRAM_SENT;
    if (trimmed > 0)
    {
        result = SolidSyslogDatagram_SendTo(udp->config.datagram, buffer, trimmed, Address(udp));
        if (result == SOLIDSYSLOG_DATAGRAM_OVERSIZE)
        {
            /* Retry still OVERSIZE means the kernel disagrees with its own
             * MaxPayload — swallow for the same reason. */
            result = SOLIDSYSLOG_DATAGRAM_SENT;
        }
    }
    return result;
}

static uint32_t NilEndpointVersion(void)
{
    return 0;
}

static bool NilUdpSenderSend(struct SolidSyslogSender* self, const void* buffer, size_t size)
{
    (void) self;
    (void) buffer;
    (void) size;
    return true;
}

static void NilUdpSenderDisconnect(struct SolidSyslogSender* self)
{
    (void) self;
}
