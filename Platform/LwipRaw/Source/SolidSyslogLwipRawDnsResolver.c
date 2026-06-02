#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogLwipRawAddressPrivate.h"
#include "SolidSyslogLwipRawDnsResolverErrors.h"
#include "SolidSyslogLwipRawDnsResolverPrivate.h"
#include "SolidSyslogLwipRawMarshalPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolverCategories.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTransport.h"
#include "SolidSyslogTunables.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"

struct SolidSyslogAddress;

/* Per-resolve parameters carried across the marshal hops. The dns_gethostbyname
 * call runs on the lwIP-owning thread; its immediate return code comes back in
 * Err. The async-completion result is published the same way: DoPublishResult
 * runs on the lwIP thread, copies the resolved address into Destination, and
 * reports the outcome in Resolved — so the multi-byte ResolvedIp / ResolvedOk
 * fields the dns_found_callback wrote are read on the thread that wrote them
 * (race-free, with the marshal providing the cross-thread barrier) rather than
 * on the caller's thread off the back of the volatile Done flag. One struct so
 * the void*-context recovery has a single cast site. */
struct LwipRawDnsResolverCall
{
    struct SolidSyslogLwipRawDnsResolver* Self;
    const char* Host;
    struct SolidSyslogLwipRawAddress* Destination;
    uint16_t Port;
    err_t Err;
    bool Resolved;
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
static bool LwipRawDnsResolver_WaitForCallback(struct SolidSyslogLwipRawDnsResolver* self);
static void LwipRawDnsResolver_DoPublishResult(void* context);

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
    struct LwipRawDnsResolverCall call = {
        .Self = self,
        .Host = host,
        .Destination = destination,
        .Port = port,
        .Err = ERR_OK,
        .Resolved = false,
    };
    SolidSyslogLwipRaw_Marshal(LwipRawDnsResolver_DoGetHostByName, &call);

    if (call.Err == ERR_OK)
    {
        /* Synchronous hit: numeric literal, DNS cache, or local hostlist —
         * dns_gethostbyname wrote ResolvedIp under the marshal hop above and the
         * found_callback never fired, so reading it here (ordered after the
         * marshal returned) is safe and on a single thread. */
        destination->Ip = self->ResolvedIp;
        destination->Port = port;
        call.Resolved = true;
    }
    else if (call.Err == ERR_INPROGRESS)
    {
        /* Query queued: bound-spin on the caller's thread until the
         * dns_found_callback signals completion via the volatile Done flag (or
         * the deadline passes). The callback wrote the multi-byte ResolvedIp /
         * ResolvedOk on the lwIP thread, so the authoritative read + publish runs
         * back on that thread via DoPublishResult — never off the volatile flag
         * on the caller's thread, which would be an unsynchronised data race. */
        if (LwipRawDnsResolver_WaitForCallback(self))
        {
            SolidSyslogLwipRaw_Marshal(LwipRawDnsResolver_DoPublishResult, &call);
        }
    }
    else
    {
        /* ERR_ARG / any other immediate rejection — Resolved stays false.
         * Terminating else per MISRA 15.7. */
    }
    return call.Resolved;
}

static void LwipRawDnsResolver_DoGetHostByName(void* context)
{
    struct LwipRawDnsResolverCall* call = LwipRawDnsResolverCallFromContext(context);
    struct SolidSyslogLwipRawDnsResolver* self = call->Self;
    call->Err = dns_gethostbyname(call->Host, &self->ResolvedIp, LwipRawDnsResolver_FoundCallback, self);
}

/* lwIP fires this on the tcpip thread when an async lookup completes. ipaddr is
 * the resolved address, or NULL on failure. Touches no lwIP API, so it needs no
 * marshalling; sets the flags the bounded spin (WaitForCallback) polls. */
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

/* Bounded async-resolve spin: each iteration sleeps via the integrator-injected
 * Sleep so lwIP's DNS timer / RX paths get cycles to advance the query. Runs on
 * the caller's thread — never the lwIP thread. Exits when the callback has set
 * the volatile Done flag (success or NULL-delivery failure) or the deadline
 * elapses (timeout). Returns whether completion was observed; the authoritative
 * success/address read happens under the marshal in DoPublishResult, so this
 * deliberately does NOT read the non-volatile ResolvedOk / ResolvedIp here. */
static bool LwipRawDnsResolver_WaitForCallback(struct SolidSyslogLwipRawDnsResolver* self)
{
    const uint32_t pollMs = (uint32_t) SOLIDSYSLOG_LWIP_RAW_DNS_RESOLVE_POLL_MS;
    const uint32_t deadlineMs = (uint32_t) SOLIDSYSLOG_DNS_RESOLVE_TIMEOUT_MS;
    uint32_t elapsedMs = 0;
    while (!self->Done && (elapsedMs < deadlineMs))
    {
        self->Config.Sleep((int) pollMs);
        elapsedMs += pollMs;
    }
    if (!self->Done)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &LwipRawDnsResolverErrorSource,
            SOLIDSYSLOG_CAT_RESOLVER_RESOLVE_FAILED,
            (int32_t) LWIPRAWDNSRESOLVER_ERROR_RESOLVE_TIMEOUT
        );
    }
    return self->Done;
}

/* Marshalled result read for the async path: runs on the lwIP thread (the same
 * thread the dns_found_callback wrote ResolvedOk / ResolvedIp on), so the read
 * is race-free and the marshal hop carries the cross-thread happens-before that
 * the volatile Done flag alone cannot. Publishes into the caller-owned
 * Destination and reports the outcome in Resolved (read after the marshal
 * returns, ordered by it). */
static void LwipRawDnsResolver_DoPublishResult(void* context)
{
    struct LwipRawDnsResolverCall* call = LwipRawDnsResolverCallFromContext(context);
    struct SolidSyslogLwipRawDnsResolver* self = call->Self;
    call->Resolved = self->ResolvedOk;
    if (self->ResolvedOk)
    {
        call->Destination->Ip = self->ResolvedIp;
        call->Destination->Port = call->Port;
    }
}

static inline struct SolidSyslogLwipRawDnsResolver* LwipRawDnsResolver_SelfFromBase(struct SolidSyslogResolver* base)
{
    return (struct SolidSyslogLwipRawDnsResolver*) base;
}

static inline struct LwipRawDnsResolverCall* LwipRawDnsResolverCallFromContext(void* context)
{
    return (struct LwipRawDnsResolverCall*) context;
}
