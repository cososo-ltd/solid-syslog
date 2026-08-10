/** @file
 *  The lwIP Raw resolver for a collector named by name, resolving names and
 *  numeric addresses alike.
 *
 *  Choosing between the two: this one where the endpoint may be a name, since a
 *  literal, a DNS-cache hit and a local-hostlist entry all resolve through it
 *  too; SolidSyslogLwipRawResolver.h where the endpoint is always an address,
 *  which needs neither LWIP_DNS nor a Sleep and takes no marshal hop. A build
 *  wires whichever one its deployment needs.
 *
 *  Resolve wraps lwIP's asynchronous dns_gethostbyname, which touches lwIP core
 *  state and so runs under the SolidSyslogLwipRaw_Marshal hop (unlike the
 *  numeric resolver's pure ipaddr_aton parse). It bridges that async call to the
 *  synchronous Resolve contract on the caller's thread:
 *
 *  - An ERR_OK synchronous hit (numeric literal, DNS cache, local hostlist)
 *    resolves immediately.
 *  - An ERR_INPROGRESS queued query spins on the caller's thread — sleeping via
 *    the config's Sleep so lwIP's DNS timer / RX paths get cycles — until the
 *    dns_found_callback fires or the deadline passes. The authoritative address
 *    read then runs back under the marshal hop (the thread the callback wrote
 *    on), never off the volatile completion flag.
 *  - Any other immediate rejection, or the deadline elapsing, fails the Resolve
 *    so the caller's unresolved-host error path runs.
 *
 *  The transport is ignored. Requires LWIP_DNS=1. See docs/platforms/lwipraw/setup.md. */
#ifndef SOLIDSYSLOGLWIPRAWDNSRESOLVER_H
#define SOLIDSYSLOGLWIPRAWDNSRESOLVER_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogSleep.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogResolver;

    /** Tunes SolidSyslogLwipRawDnsResolver's bounded async-resolve spin. */
    struct SolidSyslogLwipRawDnsResolverConfig
    {
        /** Required; each spin iteration sleeps through it so lwIP can advance the
         *  query. A NULL config or NULL Sleep falls back to the shared NullResolver. */
        SolidSyslogSleepFunction Sleep;
    };

    /** Draw a resolver from the pool; the config's Sleep drives the bounded spin
     *  (deadline SOLIDSYSLOG_DNS_RESOLVE_TIMEOUT_MS, poll
     *  SOLIDSYSLOG_LWIP_RAW_DNS_RESOLVE_POLL_MS). A NULL config, a NULL Sleep, or
     *  an exhausted pool falls back to the shared NullResolver. */
    struct SolidSyslogResolver* SolidSyslogLwipRawDnsResolver_Create(
        const struct SolidSyslogLwipRawDnsResolverConfig* config
    );
    /** Release the pool slot. */
    void SolidSyslogLwipRawDnsResolver_Destroy(struct SolidSyslogResolver * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWDNSRESOLVER_H */
