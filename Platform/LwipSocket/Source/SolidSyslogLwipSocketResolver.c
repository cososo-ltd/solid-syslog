/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

/* This component resolves names through lwIP's getaddrinfo, which the stack
   provides only when built with both the sockets layer and DNS. */
#if LWIP_SOCKET && LWIP_DNS

#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipSocketAddressPrivate.h"
#include "SolidSyslogLwipSocketResolverErrors.h"
#include "SolidSyslogLwipSocketResolverPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogResolverCategories.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTransport.h"

const struct SolidSyslogErrorSource SolidSyslogLwipSocketResolverErrorSource = {"LwipSocketResolver"};

struct SolidSyslogAddress;

enum
{
    GETADDRINFO_SUCCESS = 0
};

static bool LwipSocketResolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
);
static inline bool LwipSocketResolver_IsIpv4(const struct addrinfo* info);

void SolidSyslogLwipSocketResolver_Initialise(struct SolidSyslogResolver* base)
{
    base->Resolve = LwipSocketResolver_Resolve;
}

static bool LwipSocketResolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
)
{
    (void) base;
    /* lwIP's getaddrinfo does not act on ai_socktype, so the transport tells the
     * lookup nothing. The port is applied to the answer instead of being asked
     * for as a service name. */
    (void) transport;

    /* IPv4 only, which is what the sockaddr_in the transports send to can
     * carry. The stack narrows its own lookup on this where it is built for
     * both families; the refusal below does not depend on it doing so. */
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;

    struct addrinfo* info = NULL;
    bool resolved = false;

    if (lwip_getaddrinfo(host, NULL, &hints, &info) == GETADDRINFO_SUCCESS)
    {
        if (LwipSocketResolver_IsIpv4(info) == true)
        {
            struct sockaddr_in* sin = SolidSyslogLwipSocketAddress_AsSockaddrIn(result);
            *sin = *(const struct sockaddr_in*) (const void*) info->ai_addr;
            sin->sin_port = lwip_htons(port);
            resolved = true;
        }
        else
        {
            LwipSocketResolver_Report(
                SOLIDSYSLOG_SEVERITY_ERROR,
                SOLIDSYSLOG_CAT_RESOLVER_RESOLVE_FAILED,
                SOLIDSYSLOG_RESOLVER_ERROR_ADDRESS_FAMILY_UNSUPPORTED
            );
        }
        /* Freed on both paths: the answer comes out of a fixed pool, so a
         * refusal that keeps it spends the pool on destinations we reject. */
        lwip_freeaddrinfo(info);
    }

    return resolved;
}

static inline bool LwipSocketResolver_IsIpv4(const struct addrinfo* info)
{
    return (info->ai_family == AF_INET);
}

void SolidSyslogLwipSocketResolver_Cleanup(struct SolidSyslogResolver* base)
{
    /* Overwrite the abstract base with the shared NullResolver vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullResolver_Get();
}

#else

/* ISO C forbids an empty translation unit. */
typedef int LwipSocketResolver_EmptyTranslationUnit;

#endif /* LWIP_SOCKET && LWIP_DNS */
