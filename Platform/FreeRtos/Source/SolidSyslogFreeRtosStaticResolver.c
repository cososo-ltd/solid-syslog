#include "SolidSyslogFreeRtosStaticResolver.h"

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

#include "SolidSyslogAddressInternal.h"
#include "SolidSyslogMacros.h"
#include "SolidSyslogResolverDefinition.h"

typedef struct SolidSyslogFreeRtosStaticResolver FreeRtosStaticResolver;

struct SolidSyslogFreeRtosStaticResolver
{
    struct SolidSyslogResolver Base;
    uint8_t Octets[4];
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(FreeRtosStaticResolver) <= SOLIDSYSLOG_FREERTOSSTATICRESOLVER_SIZE,
    "SOLIDSYSLOG_FREERTOSSTATICRESOLVER_SIZE is too small for SolidSyslogFreeRtosStaticResolver layout"
);

static bool FreeRtosStaticResolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
);

static inline FreeRtosStaticResolver* FreeRtosStaticResolver_SelfFromStorage(
    SolidSyslogFreeRtosStaticResolverStorage* storage
);
static inline FreeRtosStaticResolver* FreeRtosStaticResolver_SelfFromBase(struct SolidSyslogResolver* base);

static const FreeRtosStaticResolver DEFAULT_INSTANCE = {
    {FreeRtosStaticResolver_Resolve},
    {0, 0, 0, 0},
};

static const FreeRtosStaticResolver DESTROYED_INSTANCE = {
    {NULL},
    {0, 0, 0, 0},
};

struct SolidSyslogResolver* SolidSyslogFreeRtosStaticResolver_Create(
    SolidSyslogFreeRtosStaticResolverStorage* storage,
    const uint8_t ipv4Octets[4]
)
{
    FreeRtosStaticResolver* self = FreeRtosStaticResolver_SelfFromStorage(storage);
    *self = DEFAULT_INSTANCE;
    self->Octets[0] = ipv4Octets[0];
    self->Octets[1] = ipv4Octets[1];
    self->Octets[2] = ipv4Octets[2];
    self->Octets[3] = ipv4Octets[3];
    return &self->Base;
}

static inline FreeRtosStaticResolver* FreeRtosStaticResolver_SelfFromStorage(
    SolidSyslogFreeRtosStaticResolverStorage* storage
)
{
    return (FreeRtosStaticResolver*) storage;
}

void SolidSyslogFreeRtosStaticResolver_Destroy(struct SolidSyslogResolver* base)
{
    FreeRtosStaticResolver* self = FreeRtosStaticResolver_SelfFromBase(base);
    *self = DESTROYED_INSTANCE;
}

static inline FreeRtosStaticResolver* FreeRtosStaticResolver_SelfFromBase(struct SolidSyslogResolver* base)
{
    return (FreeRtosStaticResolver*) base;
}

static bool FreeRtosStaticResolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
)
{
    (void) transport;
    (void) host;
    FreeRtosStaticResolver* self = FreeRtosStaticResolver_SelfFromBase(base);
    struct freertos_sockaddr* sockaddr = SolidSyslogAddress_AsFreertosSockaddr(result);
    sockaddr->sin_family = FREERTOS_AF_INET;
    sockaddr->sin_port = (uint16_t) FreeRTOS_htons(port);
    sockaddr->sin_address.ulIP_IPv4 =
        FreeRTOS_inet_addr_quick(self->Octets[0], self->Octets[1], self->Octets[2], self->Octets[3]);
    return true;
}
