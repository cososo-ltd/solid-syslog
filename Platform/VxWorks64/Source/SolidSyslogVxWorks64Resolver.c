/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Resolver.h"

#include <stdbool.h>
#include <stdint.h>

#include "vxWorks.h"

#include <netinet/in.h>
#include <sys/socket.h>

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
    (void) host;
    (void) port;
    struct sockaddr_in* sin = SolidSyslogVxWorks64Address_AsSockaddrIn(result);
    sin->sin_family = AF_INET;
    return true;
}
