#include "SolidSyslogWinsockResolver.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogAddressInternal.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTransport.h"
#include "SolidSyslogWinsockResolverInternal.h"
#include "SolidSyslogWinsockResolverPrivate.h"

/* File-local forwarders. Taking the address of a __declspec(dllimport)
   Winsock function for static initialisation triggers MSVC C4232 (the address
   isn't a compile-time constant); forwarding through a static function whose
   address IS a compile-time constant avoids the warning without a suppression. */
static int WSAAPI
CallGetAddrInfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res);
static void WSAAPI WinsockResolver_CallFreeAddrInfo(struct addrinfo* res);

WinsockGetAddrInfoFn Winsock_getaddrinfo = CallGetAddrInfo;
WinsockFreeAddrInfoFn Winsock_freeaddrinfo = WinsockResolver_CallFreeAddrInfo;

static int WSAAPI
CallGetAddrInfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res)
{
    return getaddrinfo(node, service, hints, res);
}

static void WSAAPI WinsockResolver_CallFreeAddrInfo(struct addrinfo* res)
{
    freeaddrinfo(res);
}

enum
{
    GETADDRINFO_SUCCESS = 0
};

static bool WinsockResolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
);
static int WinsockResolver_MapTransport(enum SolidSyslogTransport transport);

void WinsockResolver_Initialise(struct SolidSyslogResolver* base)
{
    base->Resolve = WinsockResolver_Resolve;
}

void WinsockResolver_Cleanup(struct SolidSyslogResolver* base)
{
    /* Overwrite the abstract base with the shared NullResolver vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullResolver_Get();
}

static bool WinsockResolver_Resolve(
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
    hints.ai_socktype = WinsockResolver_MapTransport(transport);

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

static int WinsockResolver_MapTransport(enum SolidSyslogTransport transport)
{
    int socktype = SOCK_DGRAM;

    if (transport == SOLIDSYSLOG_TRANSPORT_TCP)
    {
        socktype = SOCK_STREAM;
    }

    return socktype;
}
