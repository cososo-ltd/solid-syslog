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
    struct SolidSyslogResolver base;
    uint8_t                    octets[4];
};

SOLIDSYSLOG_STATIC_ASSERT(sizeof(FreeRtosStaticResolver) <= SOLIDSYSLOG_FREERTOSSTATICRESOLVER_SIZE,
                          "SOLIDSYSLOG_FREERTOSSTATICRESOLVER_SIZE is too small for SolidSyslogFreeRtosStaticResolver layout");

static bool FreeRtosStaticResolver_Resolve(struct SolidSyslogResolver* self, enum SolidSyslogTransport transport, const char* host, uint16_t port,
                                           struct SolidSyslogAddress* result);
static inline FreeRtosStaticResolver* FreeRtosStaticResolver_From(struct SolidSyslogResolver* self);

static const FreeRtosStaticResolver DEFAULT_INSTANCE = {
    {FreeRtosStaticResolver_Resolve},
    {0, 0, 0, 0},
};

static const FreeRtosStaticResolver DESTROYED_INSTANCE = {
    {NULL},
    {0, 0, 0, 0},
};

struct SolidSyslogResolver* SolidSyslogFreeRtosStaticResolver_Create(SolidSyslogFreeRtosStaticResolverStorage* storage, const uint8_t ipv4Octets[4])
{
    FreeRtosStaticResolver* self = (FreeRtosStaticResolver*) storage;
    *self                        = DEFAULT_INSTANCE;
    self->octets[0]              = ipv4Octets[0];
    self->octets[1]              = ipv4Octets[1];
    self->octets[2]              = ipv4Octets[2];
    self->octets[3]              = ipv4Octets[3];
    return &self->base;
}

void SolidSyslogFreeRtosStaticResolver_Destroy(struct SolidSyslogResolver* resolver)
{
    FreeRtosStaticResolver* self = FreeRtosStaticResolver_From(resolver);
    *self                        = DESTROYED_INSTANCE;
}

static inline FreeRtosStaticResolver* FreeRtosStaticResolver_From(struct SolidSyslogResolver* self)
{
    return (FreeRtosStaticResolver*) self;
}

static bool FreeRtosStaticResolver_Resolve(struct SolidSyslogResolver* self, enum SolidSyslogTransport transport, const char* host, uint16_t port,
                                           struct SolidSyslogAddress* result)
{
    (void) transport;
    (void) host;
    FreeRtosStaticResolver*   me       = FreeRtosStaticResolver_From(self);
    struct freertos_sockaddr* sockaddr = SolidSyslogAddress_AsFreertosSockaddr(result);
    sockaddr->sin_family               = FREERTOS_AF_INET;
    sockaddr->sin_port                 = (uint16_t) FreeRTOS_htons(port);
    sockaddr->sin_address.ulIP_IPv4    = FreeRTOS_inet_addr_quick(me->octets[0], me->octets[1], me->octets[2], me->octets[3]);
    return true;
}
