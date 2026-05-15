#include "SolidSyslogWinsockResolver.h"
#include "SolidSyslogAddressInternal.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogWinsockResolverInternal.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* File-local forwarders. Taking the address of a __declspec(dllimport)
   Winsock function for static initialisation triggers MSVC C4232 (the address
   isn't a compile-time constant); forwarding through a static function whose
   address IS a compile-time constant avoids the warning without a suppression. */
static int WSAAPI
CallGetAddrInfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res);
static void WSAAPI CallFreeAddrInfo(struct addrinfo* res);

WinsockGetAddrInfoFn Winsock_getaddrinfo = CallGetAddrInfo;
WinsockFreeAddrInfoFn Winsock_freeaddrinfo = CallFreeAddrInfo;

static int WSAAPI
CallGetAddrInfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res)
{
    return getaddrinfo(node, service, hints, res);
}

static void WSAAPI CallFreeAddrInfo(struct addrinfo* res)
{
    freeaddrinfo(res);
}

enum
{
    GETADDRINFO_SUCCESS = 0
};

static bool Resolve(
    struct SolidSyslogResolver* self,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
);
static int MapTransport(enum SolidSyslogTransport transport);

struct SolidSyslogWinsockResolver
{
    struct SolidSyslogResolver base;
};

static struct SolidSyslogWinsockResolver instance;

struct SolidSyslogResolver* SolidSyslogWinsockResolver_Create(void)
{
    instance.base.Resolve = Resolve;
    return &instance.base;
}

void SolidSyslogWinsockResolver_Destroy(void)
{
    instance.base.Resolve = NULL;
}

static bool Resolve(
    struct SolidSyslogResolver* self,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
)
{
    (void) self;

    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = MapTransport(transport);

    struct addrinfo* info = NULL;
    bool resolved = false;

    if (Winsock_getaddrinfo(host, NULL, &hints, &info) == GETADDRINFO_SUCCESS)
    {
        struct sockaddr_in* sin = SolidSyslogAddress_AsSockaddrIn(result);
        *sin = *(struct sockaddr_in*) info->ai_addr;
        sin->sin_port = htons(port);
        Winsock_freeaddrinfo(info);
        resolved = true;
    }

    return resolved;
}

static int MapTransport(enum SolidSyslogTransport transport)
{
    int socktype = SOCK_DGRAM;

    if (transport == SolidSyslogTransport_Tcp)
    {
        socktype = SOCK_STREAM;
    }

    return socktype;
}
