/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPosixResolver.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPosixResolverErrors.h"
#include "SolidSyslogPosixResolverPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogPosixAddressPrivate.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTransport.h"

const struct SolidSyslogErrorSource PosixResolverErrorSource = {"PosixResolver"};

struct SolidSyslogAddress;

enum
{
    GETADDRINFO_SUCCESS = 0
};

static bool PosixResolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
);
static int PosixResolver_MapTransport(enum SolidSyslogTransport transport);

void SolidSyslogPosixResolver_Initialise(struct SolidSyslogResolver* base)
{
    base->Resolve = PosixResolver_Resolve;
}

void SolidSyslogPosixResolver_Cleanup(struct SolidSyslogResolver* base)
{
    /* Overwrite the abstract base with the shared NullResolver vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullResolver_Get();
}

static bool PosixResolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
)
{
    (void) base;

    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = PosixResolver_MapTransport(transport);

    struct addrinfo* info = NULL;
    bool resolved = false;

    if (getaddrinfo(host, NULL, &hints, &info) == GETADDRINFO_SUCCESS)
    {
        struct sockaddr_in* sin = SolidSyslogPosixAddress_AsSockaddrIn(result);
        *sin = *(struct sockaddr_in*) info->ai_addr;
        sin->sin_port = htons(port);
        freeaddrinfo(info);
        resolved = true;
    }

    return resolved;
}

static int PosixResolver_MapTransport(enum SolidSyslogTransport transport)
{
    int socktype = SOCK_DGRAM;

    if (transport == SOLIDSYSLOG_TRANSPORT_TCP)
    {
        socktype = SOCK_STREAM;
    }

    return socktype;
}
