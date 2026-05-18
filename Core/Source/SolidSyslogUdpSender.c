#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogAddress.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogEndpoint.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogTransport.h"
#include "SolidSyslogUdpPayload.h"
#include "SolidSyslogUdpSender.h"
#include "SolidSyslogUdpSenderPrivate.h"

struct SolidSyslogFormatter;

static bool UdpSender_Send(struct SolidSyslogSender* base, const void* buffer, size_t size);
static void UdpSender_Disconnect(struct SolidSyslogSender* base);

static inline struct SolidSyslogUdpSender* UdpSender_SelfFromBase(struct SolidSyslogSender* base);
static inline bool UdpSender_Reconcile(struct SolidSyslogUdpSender* self);
static inline void UdpSender_DisconnectIfStale(struct SolidSyslogUdpSender* self);
static inline bool UdpSender_EnsureConnected(struct SolidSyslogUdpSender* self);
static inline bool UdpSender_Connected(struct SolidSyslogUdpSender* self);
static bool UdpSender_Connect(struct SolidSyslogUdpSender* self);
static inline uint16_t UdpSender_QueryEndpointPort(
    struct SolidSyslogUdpSender* self,
    struct SolidSyslogFormatter* hostFormatter
);
static inline bool UdpSender_OpenSocket(struct SolidSyslogUdpSender* self);
static bool UdpSender_ResolveDestination(struct SolidSyslogUdpSender* self, const char* host, uint16_t port);
static inline struct SolidSyslogAddress* UdpSender_Address(struct SolidSyslogUdpSender* self);
static inline void UdpSender_CloseSocket(struct SolidSyslogUdpSender* self);
static inline bool UdpSender_TransmitDatagram(struct SolidSyslogUdpSender* self, const void* buffer, size_t size);
static inline enum SolidSyslogDatagramSendResult UdpSender_RetryAfterOversize(
    struct SolidSyslogUdpSender* self,
    const void* buffer,
    size_t size
);
static uint32_t UdpSender_NilEndpointVersion(void);

void UdpSender_Initialise(struct SolidSyslogSender* base, const struct SolidSyslogUdpSenderConfig* config)
{
    struct SolidSyslogUdpSender* self = UdpSender_SelfFromBase(base);
    self->Base.Send = UdpSender_Send;
    self->Base.Disconnect = UdpSender_Disconnect;
    self->Config = *config;
    if (self->Config.EndpointVersion == NULL)
    {
        self->Config.EndpointVersion = UdpSender_NilEndpointVersion;
    }
    self->Connected = false;
    self->LastEndpointVersion = 0;
}

void UdpSender_Cleanup(struct SolidSyslogSender* base)
{
    struct SolidSyslogUdpSender* self = UdpSender_SelfFromBase(base);
    UdpSender_Disconnect(base);
    self->Base.Send = NULL;
    self->Base.Disconnect = NULL;
    self->Config.Resolver = NULL;
    self->Config.Datagram = NULL;
    self->Config.Endpoint = NULL;
    self->Config.EndpointVersion = NULL;
    self->Connected = false;
    self->LastEndpointVersion = 0;
}

static bool UdpSender_Send(struct SolidSyslogSender* base, const void* buffer, size_t size)
{
    bool result = false;
    if (buffer == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_UDPSENDER_SEND_NULL_BUFFER);
    }
    else
    {
        struct SolidSyslogUdpSender* self = UdpSender_SelfFromBase(base);
        result = UdpSender_Reconcile(self) && UdpSender_TransmitDatagram(self, buffer, size);
    }
    return result;
}

static void UdpSender_Disconnect(struct SolidSyslogSender* base)
{
    struct SolidSyslogUdpSender* self = UdpSender_SelfFromBase(base);

    if (UdpSender_Connected(self))
    {
        UdpSender_CloseSocket(self);
    }
}

static inline struct SolidSyslogUdpSender* UdpSender_SelfFromBase(struct SolidSyslogSender* base)
{
    return (struct SolidSyslogUdpSender*) base;
}

static inline bool UdpSender_Reconcile(struct SolidSyslogUdpSender* self)
{
    UdpSender_DisconnectIfStale(self);
    return UdpSender_EnsureConnected(self);
}

static inline void UdpSender_DisconnectIfStale(struct SolidSyslogUdpSender* self)
{
    uint32_t version = self->Config.EndpointVersion();

    if (version != self->LastEndpointVersion)
    {
        UdpSender_Disconnect(&self->Base);
        self->LastEndpointVersion = version;
    }
}

static inline bool UdpSender_EnsureConnected(struct SolidSyslogUdpSender* self)
{
    return UdpSender_Connected(self) || UdpSender_Connect(self);
}

static inline bool UdpSender_Connected(struct SolidSyslogUdpSender* self)
{
    return self->Connected;
}

static bool UdpSender_Connect(struct SolidSyslogUdpSender* self)
{
    SolidSyslogFormatterStorage hostStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_HOST_SIZE)];
    struct SolidSyslogFormatter* hostFormatter = SolidSyslogFormatter_Create(hostStorage, SOLIDSYSLOG_MAX_HOST_SIZE);
    uint16_t port = UdpSender_QueryEndpointPort(self, hostFormatter);
    const char* host = SolidSyslogFormatter_AsFormattedBuffer(hostFormatter);

    if (UdpSender_OpenSocket(self) && UdpSender_ResolveDestination(self, host, port))
    {
        self->Connected = true;
    }
    else
    {
        UdpSender_CloseSocket(self);
    }
    return UdpSender_Connected(self);
}

static inline uint16_t UdpSender_QueryEndpointPort(
    struct SolidSyslogUdpSender* self,
    struct SolidSyslogFormatter* hostFormatter
)
{
    struct SolidSyslogEndpoint endpoint = {.Host = hostFormatter, .Port = 0};
    self->Config.Endpoint(&endpoint);
    return endpoint.Port;
}

static inline bool UdpSender_OpenSocket(struct SolidSyslogUdpSender* self)
{
    return SolidSyslogDatagram_Open(self->Config.Datagram);
}

static bool UdpSender_ResolveDestination(struct SolidSyslogUdpSender* self, const char* host, uint16_t port)
{
    return SolidSyslogResolver_Resolve(
        self->Config.Resolver,
        SOLIDSYSLOG_TRANSPORT_UDP,
        host,
        port,
        UdpSender_Address(self)
    );
}

static inline struct SolidSyslogAddress* UdpSender_Address(struct SolidSyslogUdpSender* self)
{
    return SolidSyslogAddress_FromStorage(&self->AddrStorage);
}

static inline void UdpSender_CloseSocket(struct SolidSyslogUdpSender* self)
{
    SolidSyslogDatagram_Close(self->Config.Datagram);
    self->Connected = false;
}

static inline bool UdpSender_TransmitDatagram(struct SolidSyslogUdpSender* self, const void* buffer, size_t size)
{
    enum SolidSyslogDatagramSendResult result =
        SolidSyslogDatagram_SendTo(self->Config.Datagram, buffer, size, UdpSender_Address(self));
    if (result == SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE)
    {
        result = UdpSender_RetryAfterOversize(self, buffer, size);
    }
    return result == SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT;
}

static inline enum SolidSyslogDatagramSendResult UdpSender_RetryAfterOversize(
    struct SolidSyslogUdpSender* self,
    const void* buffer,
    size_t size
)
{
    size_t maxPayload = SolidSyslogDatagram_MaxPayload(self->Config.Datagram);
    size_t clipLimit = (size < maxPayload) ? size : maxPayload;
    size_t trimmed = SolidSyslogUdpPayload_TrimToCodepointBoundary((const uint8_t*) buffer, clipLimit);
    /* Default SENT swallows trimmed == 0 (path can't carry the message) so the
     * Service algorithm doesn't loop on an undeliverable. */
    enum SolidSyslogDatagramSendResult result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT;
    if (trimmed > 0U)
    {
        result = SolidSyslogDatagram_SendTo(self->Config.Datagram, buffer, trimmed, UdpSender_Address(self));
        if (result == SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE)
        {
            /* Retry still OVERSIZE means the kernel disagrees with its own
             * MaxPayload — swallow for the same reason. */
            result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT;
        }
    }
    return result;
}

static uint32_t UdpSender_NilEndpointVersion(void)
{
    return 0;
}
