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

static bool StreamSender_Send(struct SolidSyslogSender* base, const void* buffer, size_t size);
static void StreamSender_Disconnect(struct SolidSyslogSender* base);

static inline struct SolidSyslogStreamSender* StreamSender_SelfFromStorage(SolidSyslogStreamSenderStorage* storage);
static inline struct SolidSyslogStreamSender* StreamSender_SelfFromBase(struct SolidSyslogSender* base);

static inline bool StreamSender_Reconcile(struct SolidSyslogStreamSender* self);
static inline void StreamSender_DisconnectIfStale(struct SolidSyslogStreamSender* self);
static inline bool StreamSender_EnsureConnected(struct SolidSyslogStreamSender* self);
static inline bool StreamSender_Connected(struct SolidSyslogStreamSender* self);
static bool StreamSender_Connect(struct SolidSyslogStreamSender* self);
static bool StreamSender_ResolveDestination(struct SolidSyslogStreamSender* self, struct SolidSyslogAddress* addr);
static inline void StreamSender_CloseStream(struct SolidSyslogStreamSender* self);
static bool StreamSender_TransmitFramed(struct SolidSyslogStreamSender* self, const void* buffer, size_t size);
static struct SolidSyslogFormatter* StreamSender_FormatOctetCountingPrefix(
    SolidSyslogFormatterStorage* storage,
    size_t messageSize
);
static bool StreamSender_SendBytes(struct SolidSyslogStreamSender* self, const void* data, size_t len);
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
    struct SolidSyslogStreamSender* self = StreamSender_SelfFromStorage(storage);
    *self = DEFAULT_INSTANCE;
    self->Config.Resolver = config->Resolver;
    self->Config.Stream = config->Stream;
    if (config->Endpoint != NULL)
    {
        self->Config.Endpoint = config->Endpoint;
    }
    if (config->EndpointVersion != NULL)
    {
        self->Config.EndpointVersion = config->EndpointVersion;
    }
    return &self->Base;
}

static inline struct SolidSyslogStreamSender* StreamSender_SelfFromStorage(SolidSyslogStreamSenderStorage* storage)
{
    return (struct SolidSyslogStreamSender*) storage;
}

void SolidSyslogStreamSender_Destroy(struct SolidSyslogSender* base)
{
    struct SolidSyslogStreamSender* self = StreamSender_SelfFromBase(base);
    StreamSender_Disconnect(base);
    *self = DESTROYED_INSTANCE;
}

static inline struct SolidSyslogStreamSender* StreamSender_SelfFromBase(struct SolidSyslogSender* base)
{
    return (struct SolidSyslogStreamSender*) base;
}

static bool StreamSender_Send(struct SolidSyslogSender* base, const void* buffer, size_t size)
{
    struct SolidSyslogStreamSender* self = StreamSender_SelfFromBase(base);
    return StreamSender_Reconcile(self) && StreamSender_TransmitFramed(self, buffer, size);
}

static inline bool StreamSender_Reconcile(struct SolidSyslogStreamSender* self)
{
    StreamSender_DisconnectIfStale(self);
    return StreamSender_EnsureConnected(self);
}

static inline void StreamSender_DisconnectIfStale(struct SolidSyslogStreamSender* self)
{
    uint32_t version = self->Config.EndpointVersion();

    if (version != self->LastEndpointVersion)
    {
        StreamSender_Disconnect(&self->Base);
        self->LastEndpointVersion = version;
    }
}

static inline bool StreamSender_EnsureConnected(struct SolidSyslogStreamSender* self)
{
    return StreamSender_Connected(self) || StreamSender_Connect(self);
}

static inline bool StreamSender_Connected(struct SolidSyslogStreamSender* self)
{
    return self->Connected;
}

static bool StreamSender_Connect(struct SolidSyslogStreamSender* self)
{
    SolidSyslogAddressStorage addrStorage = {0};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&addrStorage);

    if (StreamSender_ResolveDestination(self, addr))
    {
        self->Connected = SolidSyslogStream_Open(self->Config.Stream, addr);
    }

    return StreamSender_Connected(self);
}

static bool StreamSender_ResolveDestination(struct SolidSyslogStreamSender* self, struct SolidSyslogAddress* addr)
{
    SolidSyslogFormatterStorage hostStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_HOST_SIZE)];
    struct SolidSyslogFormatter* hostFormatter = SolidSyslogFormatter_Create(hostStorage, SOLIDSYSLOG_MAX_HOST_SIZE);
    struct SolidSyslogEndpoint endpoint = {.Host = hostFormatter, .Port = 0};

    self->Config.Endpoint(&endpoint);

    return SolidSyslogResolver_Resolve(
        self->Config.Resolver,
        SolidSyslogTransport_Tcp,
        SolidSyslogFormatter_AsFormattedBuffer(hostFormatter),
        endpoint.Port,
        addr
    );
}

static void StreamSender_Disconnect(struct SolidSyslogSender* base)
{
    struct SolidSyslogStreamSender* self = StreamSender_SelfFromBase(base);

    if (StreamSender_Connected(self))
    {
        StreamSender_CloseStream(self);
    }
}

static inline void StreamSender_CloseStream(struct SolidSyslogStreamSender* self)
{
    SolidSyslogStream_Close(self->Config.Stream);
    self->Connected = false;
}

static bool StreamSender_TransmitFramed(struct SolidSyslogStreamSender* self, const void* buffer, size_t size)
{
    SolidSyslogFormatterStorage prefixStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(OCTET_COUNTING_PREFIX_CAPACITY)];
    struct SolidSyslogFormatter* prefix = StreamSender_FormatOctetCountingPrefix(prefixStorage, size);

    return StreamSender_SendBytes(
               self,
               SolidSyslogFormatter_AsFormattedBuffer(prefix),
               SolidSyslogFormatter_Length(prefix)
           ) &&
           StreamSender_SendBytes(self, buffer, size);
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

static bool StreamSender_SendBytes(struct SolidSyslogStreamSender* self, const void* data, size_t len)
{
    bool sent = SolidSyslogStream_Send(self->Config.Stream, data, len);

    if (!sent)
    {
        StreamSender_CloseStream(self);
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
