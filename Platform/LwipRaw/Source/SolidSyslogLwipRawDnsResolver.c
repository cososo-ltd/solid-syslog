#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogLwipRawAddressPrivate.h"
#include "SolidSyslogLwipRawDnsResolverPrivate.h"
#include "SolidSyslogLwipRawMarshalPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTransport.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"

struct SolidSyslogAddress;

/* Per-resolve parameters carried across the marshal hop. The dns_gethostbyname
 * call runs on the lwIP-owning thread; its immediate return code comes back in
 * Err. One struct so the void*-context recovery has a single cast site. */
struct LwipRawDnsResolverCall
{
    struct SolidSyslogLwipRawDnsResolver* Self;
    const char* Host;
    err_t Err;
};

static bool LwipRawDnsResolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
);

static inline struct SolidSyslogLwipRawDnsResolver* LwipRawDnsResolver_SelfFromBase(struct SolidSyslogResolver* base);
static inline struct LwipRawDnsResolverCall* LwipRawDnsResolverCallFromContext(void* context);
static void LwipRawDnsResolver_DoGetHostByName(void* context);
static void LwipRawDnsResolver_FoundCallback(const char* name, const ip_addr_t* ipaddr, void* arg);

void LwipRawDnsResolver_Initialise(
    struct SolidSyslogResolver* base,
    const struct SolidSyslogLwipRawDnsResolverConfig* config
)
{
    struct SolidSyslogLwipRawDnsResolver* self = LwipRawDnsResolver_SelfFromBase(base);
    base->Resolve = LwipRawDnsResolver_Resolve;
    self->Config = *config;
    self->Done = false;
    self->ResolvedOk = false;
}

void LwipRawDnsResolver_Cleanup(struct SolidSyslogResolver* base)
{
    /* Overwrite the abstract base with the shared NullResolver vtable so
     * use-after-destroy resolves cleanly to a failed-lookup error path
     * rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullResolver_Get();
}

static bool LwipRawDnsResolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
)
{
    (void) transport;
    struct SolidSyslogLwipRawDnsResolver* self = LwipRawDnsResolver_SelfFromBase(base);
    struct SolidSyslogLwipRawAddress* destination = SolidSyslogLwipRawAddress_As(result);
    self->Done = false;
    self->ResolvedOk = false;

    /* dns_gethostbyname touches lwIP core state (unlike the numeric resolver's
     * pure ipaddr_aton parse), so it must run under the marshal hop. */
    struct LwipRawDnsResolverCall call = {.Self = self, .Host = host, .Err = ERR_OK};
    SolidSyslogLwipRaw_Marshal(LwipRawDnsResolver_DoGetHostByName, &call);

    bool resolved = false;
    if (call.Err == ERR_OK)
    {
        /* Synchronous hit: numeric literal, DNS cache, or local hostlist — the
         * address was written into ResolvedIp before dns_gethostbyname returned. */
        destination->Ip = self->ResolvedIp;
        destination->Port = port;
        resolved = true;
    }
    return resolved;
}

static void LwipRawDnsResolver_DoGetHostByName(void* context)
{
    struct LwipRawDnsResolverCall* call = LwipRawDnsResolverCallFromContext(context);
    struct SolidSyslogLwipRawDnsResolver* self = call->Self;
    call->Err = dns_gethostbyname(call->Host, &self->ResolvedIp, LwipRawDnsResolver_FoundCallback, self);
}

/* lwIP fires this on the tcpip thread when an async lookup completes. ipaddr is
 * the resolved address, or NULL on failure. Touches no lwIP API, so it needs no
 * marshalling; sets the flags the bounded spin (Resolve) polls. */
static void LwipRawDnsResolver_FoundCallback(const char* name, const ip_addr_t* ipaddr, void* arg)
{
    (void) name;
    struct SolidSyslogLwipRawDnsResolver* self = (struct SolidSyslogLwipRawDnsResolver*) arg;
    if (ipaddr != NULL)
    {
        self->ResolvedIp = *ipaddr;
        self->ResolvedOk = true;
    }
    self->Done = true;
}

static inline struct SolidSyslogLwipRawDnsResolver* LwipRawDnsResolver_SelfFromBase(struct SolidSyslogResolver* base)
{
    return (struct SolidSyslogLwipRawDnsResolver*) base;
}

static inline struct LwipRawDnsResolverCall* LwipRawDnsResolverCallFromContext(void* context)
{
    return (struct LwipRawDnsResolverCall*) context;
}
