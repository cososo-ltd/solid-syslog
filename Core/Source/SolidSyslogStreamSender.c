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
    struct SolidSyslogSender Base;
    struct SolidSyslogStreamSenderConfig Config;
    bool Connected;
    uint32_t LastEndpointVersion;
};

enum
{
    UINT32_MAX_DECIMAL_DIGITS = 10,
    OCTET_COUNTING_SEPARATOR = 1,
    OCTET_COUNTING_NULL_TERMINATOR = 1,
    OCTET_COUNTING_PREFIX_CAPACITY =
        UINT32_MAX_DECIMAL_DIGITS + OCTET_COUNTING_SEPARATOR + OCTET_COUNTING_NULL_TERMINATOR
};

static bool StreamSender_Send(struct SolidSyslogSender* self, const void* buffer, size_t size);
static inline bool StreamSender_Reconcile(struct SolidSyslogStreamSender* sender);
static inline void StreamSender_DisconnectIfStale(struct SolidSyslogStreamSender* sender);
static inline bool StreamSender_EnsureConnected(struct SolidSyslogStreamSender* sender);
static inline bool StreamSender_Connected(struct SolidSyslogStreamSender* sender);
static bool StreamSender_Connect(struct SolidSyslogStreamSender* sender);
static bool StreamSender_ResolveDestination(struct SolidSyslogStreamSender* sender, struct SolidSyslogAddress* addr);
static void StreamSender_Disconnect(struct SolidSyslogSender* self);
static inline void StreamSender_CloseStream(struct SolidSyslogStreamSender* sender);
static bool StreamSender_TransmitFramed(struct SolidSyslogStreamSender* sender, const void* buffer, size_t size);
static struct SolidSyslogFormatter* StreamSender_FormatOctetCountingPrefix(
    SolidSyslogFormatterStorage* storage,
    size_t messageSize
);
static bool StreamSender_SendBytes(struct SolidSyslogStreamSender* sender, const void* data, size_t len);
static void StreamSender_NilEndpoint(struct SolidSyslogEndpoint* endpoint);
static uint32_t StreamSender_NilEndpointVersion(void);

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(struct SolidSyslogStreamSender) <= sizeof(SolidSyslogStreamSenderStorage),
    "SOLIDSYSLOG_STREAM_SENDER_SIZE is too small for struct SolidSyslogStreamSender"
);

static const struct SolidSyslogStreamSender DEFAULT_INSTANCE = {
    {StreamSender_Send, StreamSender_Disconnect},
    {NULL, NULL, StreamSender_NilEndpoint, StreamSender_NilEndpointVersion},
    false,
    0,
};

static const struct SolidSyslogStreamSender DESTROYED_INSTANCE = {
    {NULL, NULL},
    {NULL, NULL, StreamSender_NilEndpoint, StreamSender_NilEndpointVersion},
    false,
    0,
};

struct SolidSyslogSender* SolidSyslogStreamSender_Create(
    SolidSyslogStreamSenderStorage* storage,
    const struct SolidSyslogStreamSenderConfig* config
)
{
    struct SolidSyslogStreamSender* sender = (struct SolidSyslogStreamSender*) storage;
    *sender = DEFAULT_INSTANCE;
    sender->Config.Resolver = config->Resolver;
    sender->Config.Stream = config->Stream;
    if (config->Endpoint != NULL)
    {
        sender->Config.Endpoint = config->Endpoint;
    }
    if (config->EndpointVersion != NULL)
    {
        sender->Config.EndpointVersion = config->EndpointVersion;
    }
    return &sender->Base;
}

void SolidSyslogStreamSender_Destroy(struct SolidSyslogSender* sender)
{
    struct SolidSyslogStreamSender* self = (struct SolidSyslogStreamSender*) sender;
    StreamSender_Disconnect(sender);
    *self = DESTROYED_INSTANCE;
}

static bool StreamSender_Send(struct SolidSyslogSender* self, const void* buffer, size_t size)
{
    struct SolidSyslogStreamSender* sender = (struct SolidSyslogStreamSender*) self;
    return StreamSender_Reconcile(sender) && StreamSender_TransmitFramed(sender, buffer, size);
}

static inline bool StreamSender_Reconcile(struct SolidSyslogStreamSender* sender)
{
    StreamSender_DisconnectIfStale(sender);
    return StreamSender_EnsureConnected(sender);
}

static inline void StreamSender_DisconnectIfStale(struct SolidSyslogStreamSender* sender)
{
    uint32_t version = sender->Config.EndpointVersion();

    if (version != sender->LastEndpointVersion)
    {
        StreamSender_Disconnect(&sender->Base);
        sender->LastEndpointVersion = version;
    }
}

static inline bool StreamSender_EnsureConnected(struct SolidSyslogStreamSender* sender)
{
    return StreamSender_Connected(sender) || StreamSender_Connect(sender);
}

static inline bool StreamSender_Connected(struct SolidSyslogStreamSender* sender)
{
    return sender->Connected;
}

static bool StreamSender_Connect(struct SolidSyslogStreamSender* sender)
{
    SolidSyslogAddressStorage addrStorage = {0};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&addrStorage);

    if (StreamSender_ResolveDestination(sender, addr))
    {
        sender->Connected = SolidSyslogStream_Open(sender->Config.Stream, addr);
    }

    return StreamSender_Connected(sender);
}

static bool StreamSender_ResolveDestination(struct SolidSyslogStreamSender* sender, struct SolidSyslogAddress* addr)
{
    SolidSyslogFormatterStorage hostStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_HOST_SIZE)];
    struct SolidSyslogFormatter* hostFormatter = SolidSyslogFormatter_Create(hostStorage, SOLIDSYSLOG_MAX_HOST_SIZE);
    struct SolidSyslogEndpoint endpoint = {.Host = hostFormatter, .Port = 0};

    sender->Config.Endpoint(&endpoint);

    return SolidSyslogResolver_Resolve(
        sender->Config.Resolver,
        SolidSyslogTransport_Tcp,
        SolidSyslogFormatter_AsFormattedBuffer(hostFormatter),
        endpoint.Port,
        addr
    );
}

static void StreamSender_Disconnect(struct SolidSyslogSender* self)
{
    struct SolidSyslogStreamSender* sender = (struct SolidSyslogStreamSender*) self;

    if (StreamSender_Connected(sender))
    {
        StreamSender_CloseStream(sender);
    }
}

static inline void StreamSender_CloseStream(struct SolidSyslogStreamSender* sender)
{
    SolidSyslogStream_Close(sender->Config.Stream);
    sender->Connected = false;
}

static bool StreamSender_TransmitFramed(struct SolidSyslogStreamSender* sender, const void* buffer, size_t size)
{
    SolidSyslogFormatterStorage prefixStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(OCTET_COUNTING_PREFIX_CAPACITY)];
    struct SolidSyslogFormatter* prefix = StreamSender_FormatOctetCountingPrefix(prefixStorage, size);

    return StreamSender_SendBytes(
               sender,
               SolidSyslogFormatter_AsFormattedBuffer(prefix),
               SolidSyslogFormatter_Length(prefix)
           ) &&
           StreamSender_SendBytes(sender, buffer, size);
}

static struct SolidSyslogFormatter* StreamSender_FormatOctetCountingPrefix(
    SolidSyslogFormatterStorage* storage,
    size_t messageSize
)
{
    struct SolidSyslogFormatter* f = SolidSyslogFormatter_Create(storage, OCTET_COUNTING_PREFIX_CAPACITY);
    SolidSyslogFormatter_Uint32(f, (uint32_t) messageSize);
    SolidSyslogFormatter_AsciiCharacter(f, ' ');
    return f;
}

static bool StreamSender_SendBytes(struct SolidSyslogStreamSender* sender, const void* data, size_t len)
{
    bool sent = SolidSyslogStream_Send(sender->Config.Stream, data, len);

    if (!sent)
    {
        StreamSender_CloseStream(sender);
    }

    return sent;
}

static void StreamSender_NilEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    SolidSyslogFormatter_BoundedString(endpoint->Host, "", 0);
    endpoint->Port = 0;
}

static uint32_t StreamSender_NilEndpointVersion(void)
{
    return 0;
}
