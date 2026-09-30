/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Resolver.h"

#include <stdbool.h>
#include <stdint.h>

#include "vxWorks.h"

#include <netinet/in.h>
#include <sys/socket.h>

#include "hostLib.h"
#include "inetLib.h"

#include "SolidSyslogError.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTransport.h"
#include "SolidSyslogVxWorks64AddressPrivate.h"
#include "SolidSyslogVxWorks64ResolverErrors.h"
#include "SolidSyslogVxWorks64ResolverPrivate.h"

const struct SolidSyslogErrorSource SolidSyslogVxWorks64ResolverErrorSource = {"VxWorks64Resolver"};

struct SolidSyslogAddress;

static bool VxWorks64Resolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
);

static inline bool VxWorks64Resolver_IsUnresolved(uint32_t address);

void SolidSyslogVxWorks64Resolver_Initialise(struct SolidSyslogResolver* base)
{
    base->Resolve = VxWorks64Resolver_Resolve;
}

void SolidSyslogVxWorks64Resolver_Cleanup(struct SolidSyslogResolver* base)
{
    (void) base;
}

static bool VxWorks64Resolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
)
{
    (void) base;
    (void) transport;
    /* Both libraries take a non-const string they do not modify (D.006), and
     * both answer ERROR - all ones as an address - for a host they cannot
     * resolve. */
    uint32_t found = (uint32_t) inet_addr((char*) host);
    if (VxWorks64Resolver_IsUnresolved(found) == true)
    {
        found = (uint32_t) hostGetByName((char*) host);
    }
    bool resolved = VxWorks64Resolver_IsUnresolved(found) == false;
    if (resolved == true)
    {
        struct sockaddr_in* sin = SolidSyslogVxWorks64Address_AsSockaddrIn(result);
        sin->sin_family = AF_INET;
        sin->sin_port = htons(port);
        sin->sin_addr.s_addr = found;
    }
    return resolved;
}

static inline bool VxWorks64Resolver_IsUnresolved(uint32_t address)
{
    return address == (uint32_t) ERROR;
}
