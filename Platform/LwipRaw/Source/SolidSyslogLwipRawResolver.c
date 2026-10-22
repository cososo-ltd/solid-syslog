/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipRawAddressPrivate.h"
#include "SolidSyslogLwipRawResolverErrors.h"
#include "SolidSyslogLwipRawResolverPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolverCategories.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTransport.h"
#include "lwip/ip_addr.h"

const struct SolidSyslogErrorSource SolidSyslogLwipRawResolverErrorSource = {
    "LwipRawResolver",
    SolidSyslogResolverErrors_Name
};

struct SolidSyslogAddress;

static bool LwipRawResolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
);

void SolidSyslogLwipRawResolver_Initialise(struct SolidSyslogResolver* base)
{
    base->Resolve = LwipRawResolver_Resolve;
}

void SolidSyslogLwipRawResolver_Cleanup(struct SolidSyslogResolver* base)
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
     * The sibling SolidSyslogLwipRawDnsResolver calls lwIP DNS APIs
     * (dns_gethostbyname) and must marshal them - that is where the hop
     * belongs, not here. */
    ip_addr_t parsed;
    bool resolved = false;
    if (ipaddr_aton(host, &parsed) != 0)
    {
        /* IPv4 only, which is what the datagram's and stream's pcbs send to.
         * A dual-stack lwIP parses an IPv6 literal too, and storing it would
         * give the transports a destination they fail every send to. */
        if (LwipRawAddress_IsIpv4(&parsed))
        {
            self->Ip = parsed;
            self->Port = port;
            resolved = true;
        }
        else
        {
            LwipRawResolver_Report(
                SOLIDSYSLOG_SEVERITY_ERROR,
                SOLIDSYSLOG_CAT_RESOLVER_RESOLVE_FAILED,
                SOLIDSYSLOG_RESOLVER_ERROR_ADDRESS_FAMILY_UNSUPPORTED
            );
        }
    }
    return resolved;
}
