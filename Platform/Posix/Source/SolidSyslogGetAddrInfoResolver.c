#include "SolidSyslogGetAddrInfoResolver.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>

#include "SolidSyslogAddressInternal.h"
#include "SolidSyslogGetAddrInfoResolverPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTransport.h"

struct SolidSyslogAddress;

enum
{
    GETADDRINFO_SUCCESS = 0
};

static bool GetAddrInfoResolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
);
static int GetAddrInfoResolver_MapTransport(enum SolidSyslogTransport transport);

void GetAddrInfoResolver_Initialise(struct SolidSyslogResolver* base)
{
    base->Resolve = GetAddrInfoResolver_Resolve;
}

void GetAddrInfoResolver_Cleanup(struct SolidSyslogResolver* base)
{
    /* Overwrite the abstract base with the shared NullResolver vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullResolver_Get();
}

static bool GetAddrInfoResolver_Resolve(
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
    hints.ai_socktype = GetAddrInfoResolver_MapTransport(transport);

    struct addrinfo* info = NULL;
    bool resolved = false;

    if (getaddrinfo(host, NULL, &hints, &info) == GETADDRINFO_SUCCESS)
    {
        struct sockaddr_in* sin = SolidSyslogAddress_AsSockaddrIn(result);
        *sin = *(struct sockaddr_in*) info->ai_addr;
        sin->sin_port = htons(port);
        freeaddrinfo(info);
        resolved = true;
    }

    return resolved;
}

static int GetAddrInfoResolver_MapTransport(enum SolidSyslogTransport transport)
{
    int socktype = SOCK_DGRAM;

    if (transport == SOLIDSYSLOG_TRANSPORT_TCP)
    {
        socktype = SOCK_STREAM;
    }

    return socktype;
}
