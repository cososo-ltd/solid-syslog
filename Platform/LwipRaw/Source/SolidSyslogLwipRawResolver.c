#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogLwipRawResolverPrivate.h"
#include "lwip/ip_addr.h"
#include "SolidSyslogLwipRawAddressPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTransport.h"

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
    /* No SolidSyslogLwipRaw_Marshal hop here: ipaddr_aton is a pure string
     * parser that touches no lwIP core state, so it is safe on any thread.
     * The sibling SolidSyslogLwipRawDnsResolver (S28.08) WILL call lwIP DNS
     * APIs (dns_gethostbyname) and must marshal them — that is where the hop
     * belongs, not here. */
    bool resolved = ipaddr_aton(host, &self->Ip) != 0;
    if (resolved)
    {
        self->Port = port;
    }
    return resolved;
}
