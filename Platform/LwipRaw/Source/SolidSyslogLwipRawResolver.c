#include "SolidSyslogLwipRawResolverPrivate.h"

#include <stdbool.h>
#include <stdint.h>

#include "lwip/ip_addr.h"

#include "SolidSyslogLwipRawAddressPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogResolverDefinition.h"

struct SolidSyslogAddress;

static bool LwipRawResolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
);

void LwipRawResolver_Initialise(struct SolidSyslogResolver* base)
{
    base->Resolve = LwipRawResolver_Resolve;
}

void LwipRawResolver_Cleanup(struct SolidSyslogResolver* base)
{
    /* Overwrite the abstract base with the shared NullResolver vtable so
     * use-after-destroy resolves cleanly to a failed-lookup error path
     * rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullResolver_Get();
}

static bool LwipRawResolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
)
{
    (void) base;
    (void) transport;
    struct SolidSyslogLwipRawAddress* self = SolidSyslogLwipRawAddress_As(result);
    bool resolved = ipaddr_aton(host, &self->Ip) != 0;
    if (resolved)
    {
        self->Port = port;
    }
    return resolved;
}
