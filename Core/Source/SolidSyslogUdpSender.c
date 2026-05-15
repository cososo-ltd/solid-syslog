#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "SolidSyslogEndpoint.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogUdpPayload.h"
#include "SolidSyslogUdpSender.h"
#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogAddress.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogTransport.h"

struct SolidSyslogFormatter;

struct SolidSyslogUdpSender
{
    struct SolidSyslogSender base;
    struct SolidSyslogUdpSenderConfig config;
    SolidSyslogAddressStorage addrStorage;
    bool connected;
    uint32_t lastEndpointVersion;
};

static bool UdpSender_IsValidConfig(const struct SolidSyslogUdpSenderConfig* config);
static void UdpSender_InstallConfig(const struct SolidSyslogUdpSenderConfig* config);
static bool UdpSender_Send(struct SolidSyslogSender* self, const void* buffer, size_t size);
static void UdpSender_Disconnect(struct SolidSyslogSender* self);
static inline bool UdpSender_Reconcile(struct SolidSyslogUdpSender* udp);
static inline void UdpSender_DisconnectIfStale(struct SolidSyslogUdpSender* udp);
static inline bool UdpSender_EnsureConnected(struct SolidSyslogUdpSender* udp);
static inline bool UdpSender_Connected(struct SolidSyslogUdpSender* udp);
static bool UdpSender_Connect(struct SolidSyslogUdpSender* udp);
static inline uint16_t UdpSender_QueryEndpointPort(
    struct SolidSyslogUdpSender* udp,
    struct SolidSyslogFormatter* hostFormatter
);
static inline bool UdpSender_OpenSocket(struct SolidSyslogUdpSender* udp);
static bool UdpSender_ResolveDestination(struct SolidSyslogUdpSender* udp, const char* host, uint16_t port);
static inline struct SolidSyslogAddress* UdpSender_Address(struct SolidSyslogUdpSender* udp);
static inline void UdpSender_CloseSocket(struct SolidSyslogUdpSender* udp);
static inline bool UdpSender_TransmitDatagram(struct SolidSyslogUdpSender* udp, const void* buffer, size_t size);
static inline enum SolidSyslogDatagramSendResult UdpSender_RetryAfterOversize(
    struct SolidSyslogUdpSender* udp,
    const void* buffer,
    size_t size
);
static uint32_t UdpSender_NilEndpointVersion(void);
static bool UdpSender_NilUdpSenderSend(struct SolidSyslogSender* self, const void* buffer, size_t size);
static void UdpSender_NilUdpSenderDisconnect(struct SolidSyslogSender* self);

static const struct SolidSyslogUdpSender DEFAULT_INSTANCE = {
    .config = {.endpointVersion = UdpSender_NilEndpointVersion}
};
static struct SolidSyslogUdpSender instance;
static struct SolidSyslogSender NilUdpSender = {
    .Send = UdpSender_NilUdpSenderSend,
    .Disconnect = UdpSender_NilUdpSenderDisconnect
};

struct SolidSyslogSender* SolidSyslogUdpSender_Create(const struct SolidSyslogUdpSenderConfig* config)
{
    struct SolidSyslogSender* result = &NilUdpSender;
    if (UdpSender_IsValidConfig(config))
    {
        UdpSender_InstallConfig(config);
        result = &instance.base;
    }
    return result;
}

static bool UdpSender_IsValidConfig(const struct SolidSyslogUdpSenderConfig* config)
{
    bool valid = false;
    if (config == NULL)
    {
        SolidSyslog_Error(SolidSyslogSeverity_Error, SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_CONFIG);
    }
    else if (config->resolver == NULL)
    {
        SolidSyslog_Error(SolidSyslogSeverity_Error, SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_RESOLVER);
    }
    else if (config->datagram == NULL)
    {
        SolidSyslog_Error(SolidSyslogSeverity_Error, SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_DATAGRAM);
    }
    else if (config->endpoint == NULL)
    {
        SolidSyslog_Error(SolidSyslogSeverity_Error, SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_ENDPOINT);
    }
    else
    {
        valid = true;
    }
    return valid;
}

static void UdpSender_InstallConfig(const struct SolidSyslogUdpSenderConfig* config)
{
    instance = DEFAULT_INSTANCE;
    instance.config = *config;
    if (instance.config.endpointVersion == NULL)
    {
        instance.config.endpointVersion = UdpSender_NilEndpointVersion;
    }
    instance.base.Send = UdpSender_Send;
    instance.base.Disconnect = UdpSender_Disconnect;
}

void SolidSyslogUdpSender_Destroy(void)
{
    UdpSender_Disconnect(&instance.base);
    instance = DEFAULT_INSTANCE;
}

static bool UdpSender_Send(struct SolidSyslogSender* self, const void* buffer, size_t size)
{
    bool result = false;
    if (buffer == NULL)
    {
        SolidSyslog_Error(SolidSyslogSeverity_Error, SOLIDSYSLOG_ERROR_MSG_UDPSENDER_SEND_NULL_BUFFER);
    }
    else
    {
        struct SolidSyslogUdpSender* udp = (struct SolidSyslogUdpSender*) self;
        result = UdpSender_Reconcile(udp) && UdpSender_TransmitDatagram(udp, buffer, size);
    }
    return result;
}

static void UdpSender_Disconnect(struct SolidSyslogSender* self)
{
    struct SolidSyslogUdpSender* udp = (struct SolidSyslogUdpSender*) self;

    if (UdpSender_Connected(udp))
    {
        UdpSender_CloseSocket(udp);
    }
}

static inline bool UdpSender_Reconcile(struct SolidSyslogUdpSender* udp)
{
    UdpSender_DisconnectIfStale(udp);
    return UdpSender_EnsureConnected(udp);
}

static inline void UdpSender_DisconnectIfStale(struct SolidSyslogUdpSender* udp)
{
    uint32_t version = udp->config.endpointVersion();

    if (version != udp->lastEndpointVersion)
    {
        UdpSender_Disconnect(&udp->base);
        udp->lastEndpointVersion = version;
    }
}

static inline bool UdpSender_EnsureConnected(struct SolidSyslogUdpSender* udp)
{
    return UdpSender_Connected(udp) || UdpSender_Connect(udp);
}

static inline bool UdpSender_Connected(struct SolidSyslogUdpSender* udp)
{
    return udp->connected;
}

static bool UdpSender_Connect(struct SolidSyslogUdpSender* udp)
{
    SolidSyslogFormatterStorage hostStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_HOST_SIZE)];
    struct SolidSyslogFormatter* hostFormatter = SolidSyslogFormatter_Create(hostStorage, SOLIDSYSLOG_MAX_HOST_SIZE);
    uint16_t port = UdpSender_QueryEndpointPort(udp, hostFormatter);
    const char* host = SolidSyslogFormatter_AsFormattedBuffer(hostFormatter);

    if (UdpSender_OpenSocket(udp) && UdpSender_ResolveDestination(udp, host, port))
    {
        udp->connected = true;
    }
    else
    {
        UdpSender_CloseSocket(udp);
    }
    return UdpSender_Connected(udp);
}

static inline uint16_t UdpSender_QueryEndpointPort(
    struct SolidSyslogUdpSender* udp,
    struct SolidSyslogFormatter* hostFormatter
)
{
    struct SolidSyslogEndpoint endpoint = {.host = hostFormatter, .port = 0};
    udp->config.endpoint(&endpoint);
    return endpoint.port;
}

static inline bool UdpSender_OpenSocket(struct SolidSyslogUdpSender* udp)
{
    return SolidSyslogDatagram_Open(udp->config.datagram);
}

static bool UdpSender_ResolveDestination(struct SolidSyslogUdpSender* udp, const char* host, uint16_t port)
{
    return SolidSyslogResolver_Resolve(
        udp->config.resolver,
        SolidSyslogTransport_Udp,
        host,
        port,
        UdpSender_Address(udp)
    );
}

static inline struct SolidSyslogAddress* UdpSender_Address(struct SolidSyslogUdpSender* udp)
{
    return SolidSyslogAddress_FromStorage(&udp->addrStorage);
}

static inline void UdpSender_CloseSocket(struct SolidSyslogUdpSender* udp)
{
    SolidSyslogDatagram_Close(udp->config.datagram);
    udp->connected = false;
}

static inline bool UdpSender_TransmitDatagram(struct SolidSyslogUdpSender* udp, const void* buffer, size_t size)
{
    enum SolidSyslogDatagramSendResult result =
        SolidSyslogDatagram_SendTo(udp->config.datagram, buffer, size, UdpSender_Address(udp));
    if (result == SolidSyslogDatagramSendResult_Oversize)
    {
        result = UdpSender_RetryAfterOversize(udp, buffer, size);
    }
    return result == SolidSyslogDatagramSendResult_Sent;
}

static inline enum SolidSyslogDatagramSendResult UdpSender_RetryAfterOversize(
    struct SolidSyslogUdpSender* udp,
    const void* buffer,
    size_t size
)
{
    size_t maxPayload = SolidSyslogDatagram_MaxPayload(udp->config.datagram);
    size_t clipLimit = (size < maxPayload) ? size : maxPayload;
    size_t trimmed = SolidSyslogUdpPayload_TrimToCodepointBoundary((const uint8_t*) buffer, clipLimit);
    /* Default SENT swallows trimmed == 0 (path can't carry the message) so the
     * Service algorithm doesn't loop on an undeliverable. */
    enum SolidSyslogDatagramSendResult result = SolidSyslogDatagramSendResult_Sent;
    if (trimmed > 0)
    {
        result = SolidSyslogDatagram_SendTo(udp->config.datagram, buffer, trimmed, UdpSender_Address(udp));
        if (result == SolidSyslogDatagramSendResult_Oversize)
        {
            /* Retry still OVERSIZE means the kernel disagrees with its own
             * MaxPayload — swallow for the same reason. */
            result = SolidSyslogDatagramSendResult_Sent;
        }
    }
    return result;
}

static uint32_t UdpSender_NilEndpointVersion(void)
{
    return 0;
}

static bool UdpSender_NilUdpSenderSend(struct SolidSyslogSender* self, const void* buffer, size_t size)
{
    (void) self;
    (void) buffer;
    (void) size;
    return true;
}

static void UdpSender_NilUdpSenderDisconnect(struct SolidSyslogSender* self)
{
    (void) self;
}
