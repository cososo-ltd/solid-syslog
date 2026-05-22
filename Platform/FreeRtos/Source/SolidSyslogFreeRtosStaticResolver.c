#include "SolidSyslogFreeRtosStaticResolver.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

#include "SolidSyslogFreeRtosAddressPrivate.h"
#include "SolidSyslogFreeRtosStaticResolverPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogResolverDefinition.h"

static bool FreeRtosStaticResolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
);

static inline struct SolidSyslogFreeRtosStaticResolver* FreeRtosStaticResolver_SelfFromBase(
    struct SolidSyslogResolver* base
);

void FreeRtosStaticResolver_Initialise(struct SolidSyslogResolver* base, const uint8_t ipv4Octets[4])
{
    struct SolidSyslogFreeRtosStaticResolver* self = FreeRtosStaticResolver_SelfFromBase(base);
    self->Base.Resolve = FreeRtosStaticResolver_Resolve;
    self->Octets[0] = ipv4Octets[0];
    self->Octets[1] = ipv4Octets[1];
    self->Octets[2] = ipv4Octets[2];
    self->Octets[3] = ipv4Octets[3];
}

static inline struct SolidSyslogFreeRtosStaticResolver* FreeRtosStaticResolver_SelfFromBase(
    struct SolidSyslogResolver* base
)
{
    return (struct SolidSyslogFreeRtosStaticResolver*) base;
}

void FreeRtosStaticResolver_Cleanup(struct SolidSyslogResolver* base)
{
    /* Overwrite the abstract base with the shared NullResolver vtable so
     * use-after-destroy resolves cleanly to a failed-lookup error path
     * rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullResolver_Get();
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
    struct SolidSyslogFreeRtosStaticResolver* self = FreeRtosStaticResolver_SelfFromBase(base);
    struct freertos_sockaddr* sockaddr = SolidSyslogFreeRtosAddress_AsFreertosSockaddr(result);
    sockaddr->sin_family = FREERTOS_AF_INET;
    sockaddr->sin_port = (uint16_t) FreeRTOS_htons(port);
    sockaddr->sin_address.ulIP_IPv4 =
        FreeRTOS_inet_addr_quick(self->Octets[0], self->Octets[1], self->Octets[2], self->Octets[3]);
    return true;
}
