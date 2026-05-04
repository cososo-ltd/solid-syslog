#include "SolidSyslogStreamSender.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogEndpoint.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogMacros.h"
#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogAddress.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogTransport.h"

struct SolidSyslogAddress;

struct SolidSyslogStreamSender
{
    struct SolidSyslogSender             base;
    struct SolidSyslogStreamSenderConfig config;
    bool                                 connected;
    uint32_t                             lastEndpointVersion;
};

enum
{
    UINT32_MAX_DECIMAL_DIGITS      = 10,
    OCTET_COUNTING_SEPARATOR       = 1,
    OCTET_COUNTING_NULL_TERMINATOR = 1,
    OCTET_COUNTING_PREFIX_CAPACITY = UINT32_MAX_DECIMAL_DIGITS + OCTET_COUNTING_SEPARATOR + OCTET_COUNTING_NULL_TERMINATOR
};

static bool                         Send(struct SolidSyslogSender* self, const void* buffer, size_t size);
static inline bool                  Reconcile(struct SolidSyslogStreamSender* sender);
static inline void                  DisconnectIfStale(struct SolidSyslogStreamSender* sender);
static inline bool                  EnsureConnected(struct SolidSyslogStreamSender* sender);
static inline bool                  Connected(struct SolidSyslogStreamSender* sender);
static bool                         Connect(struct SolidSyslogStreamSender* sender);
static bool                         ResolveDestination(struct SolidSyslogStreamSender* sender, struct SolidSyslogAddress* addr);
static void                         Disconnect(struct SolidSyslogSender* self);
static inline void                  CloseStream(struct SolidSyslogStreamSender* sender);
static bool                         TransmitFramed(struct SolidSyslogStreamSender* sender, const void* buffer, size_t size);
static struct SolidSyslogFormatter* FormatOctetCountingPrefix(SolidSyslogFormatterStorage* storage, size_t messageSize);
static bool                         SendBytes(struct SolidSyslogStreamSender* sender, const void* data, size_t len);
static void                         NilEndpoint(struct SolidSyslogEndpoint* endpoint);
static uint32_t                     NilEndpointVersion(void);

SOLIDSYSLOG_STATIC_ASSERT(sizeof(struct SolidSyslogStreamSender) <= sizeof(SolidSyslogStreamSenderStorage),
                          "SOLIDSYSLOG_STREAM_SENDER_SIZE is too small for struct SolidSyslogStreamSender");

static const struct SolidSyslogStreamSender DEFAULT_INSTANCE = {
    {Send, Disconnect},
    {NULL, NULL, NilEndpoint, NilEndpointVersion},
    false,
    0,
};

static const struct SolidSyslogStreamSender DESTROYED_INSTANCE = {
    {NULL, NULL},
    {NULL, NULL, NilEndpoint, NilEndpointVersion},
    false,
    0,
};

struct SolidSyslogSender* SolidSyslogStreamSender_Create(SolidSyslogStreamSenderStorage* storage, const struct SolidSyslogStreamSenderConfig* config)
{
    struct SolidSyslogStreamSender* sender = (struct SolidSyslogStreamSender*) storage;
    *sender                                = DEFAULT_INSTANCE;
    sender->config.resolver                = config->resolver;
    sender->config.stream                  = config->stream;
    ASSIGN_IF_NON_NULL(sender->config.endpoint, config->endpoint);
    ASSIGN_IF_NON_NULL(sender->config.endpointVersion, config->endpointVersion);
    return &sender->base;
}

void SolidSyslogStreamSender_Destroy(struct SolidSyslogSender* sender)
{
    struct SolidSyslogStreamSender* self = (struct SolidSyslogStreamSender*) sender;
    Disconnect(sender);
    *self = DESTROYED_INSTANCE;
}

static bool Send(struct SolidSyslogSender* self, const void* buffer, size_t size)
{
    struct SolidSyslogStreamSender* sender = (struct SolidSyslogStreamSender*) self;
    return Reconcile(sender) && TransmitFramed(sender, buffer, size);
}

static inline bool Reconcile(struct SolidSyslogStreamSender* sender)
{
    DisconnectIfStale(sender);
    return EnsureConnected(sender);
}

static inline void DisconnectIfStale(struct SolidSyslogStreamSender* sender)
{
    uint32_t version = sender->config.endpointVersion();

    if (version != sender->lastEndpointVersion)
    {
        Disconnect(&sender->base);
        sender->lastEndpointVersion = version;
    }
}

static inline bool EnsureConnected(struct SolidSyslogStreamSender* sender)
{
    return Connected(sender) || Connect(sender);
}

static inline bool Connected(struct SolidSyslogStreamSender* sender)
{
    return sender->connected;
}

static bool Connect(struct SolidSyslogStreamSender* sender)
{
    SolidSyslogAddressStorage  addrStorage = {0};
    struct SolidSyslogAddress* addr        = SolidSyslogAddress_FromStorage(&addrStorage);

    if (ResolveDestination(sender, addr))
    {
        sender->connected = SolidSyslogStream_Open(sender->config.stream, addr);
    }

    return Connected(sender);
}

static bool ResolveDestination(struct SolidSyslogStreamSender* sender, struct SolidSyslogAddress* addr)
{
    SolidSyslogFormatterStorage  hostStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_HOST_SIZE)];
    struct SolidSyslogFormatter* hostFormatter = SolidSyslogFormatter_Create(hostStorage, SOLIDSYSLOG_MAX_HOST_SIZE);
    struct SolidSyslogEndpoint   endpoint      = {.host = hostFormatter, .port = 0};

    sender->config.endpoint(&endpoint);

    return SolidSyslogResolver_Resolve(sender->config.resolver, SOLIDSYSLOG_TRANSPORT_TCP, SolidSyslogFormatter_AsFormattedBuffer(hostFormatter), endpoint.port,
                                       addr);
}

static void Disconnect(struct SolidSyslogSender* self)
{
    struct SolidSyslogStreamSender* sender = (struct SolidSyslogStreamSender*) self;

    if (Connected(sender))
    {
        CloseStream(sender);
    }
}

static inline void CloseStream(struct SolidSyslogStreamSender* sender)
{
    SolidSyslogStream_Close(sender->config.stream);
    sender->connected = false;
}

static bool TransmitFramed(struct SolidSyslogStreamSender* sender, const void* buffer, size_t size)
{
    SolidSyslogFormatterStorage  prefixStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(OCTET_COUNTING_PREFIX_CAPACITY)];
    struct SolidSyslogFormatter* prefix = FormatOctetCountingPrefix(prefixStorage, size);

    return SendBytes(sender, SolidSyslogFormatter_AsFormattedBuffer(prefix), SolidSyslogFormatter_Length(prefix)) && SendBytes(sender, buffer, size);
}

static struct SolidSyslogFormatter* FormatOctetCountingPrefix(SolidSyslogFormatterStorage* storage, size_t messageSize)
{
    struct SolidSyslogFormatter* f = SolidSyslogFormatter_Create(storage, OCTET_COUNTING_PREFIX_CAPACITY);
    SolidSyslogFormatter_Uint32(f, (uint32_t) messageSize);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    return f;
}

static bool SendBytes(struct SolidSyslogStreamSender* sender, const void* data, size_t len)
{
    bool sent = SolidSyslogStream_Send(sender->config.stream, data, len);

    if (!sent)
    {
        CloseStream(sender);
    }

    return sent;
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
