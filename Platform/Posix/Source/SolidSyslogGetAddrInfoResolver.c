#include "SolidSyslogGetAddrInfoResolver.h"

#include <netdb.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>

#include "SolidSyslogAddressInternal.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTransport.h"

struct SolidSyslogAddress;

enum
{
    GETADDRINFO_SUCCESS = 0
};

static bool Resolve(struct SolidSyslogResolver* self, enum SolidSyslogTransport transport, const char* host, uint16_t port, struct SolidSyslogAddress* result);
static int  MapTransport(enum SolidSyslogTransport transport);

struct SolidSyslogGetAddrInfoResolver
{
    struct SolidSyslogResolver base;
};

static struct SolidSyslogGetAddrInfoResolver instance;

struct SolidSyslogResolver* SolidSyslogGetAddrInfoResolver_Create(void)
{
    instance.base.Resolve = Resolve;
    return &instance.base;
}

void SolidSyslogGetAddrInfoResolver_Destroy(void)
{
    instance.base.Resolve = NULL;
}

static bool Resolve(struct SolidSyslogResolver* self, enum SolidSyslogTransport transport, const char* host, uint16_t port, struct SolidSyslogAddress* result)
{
    (void) self;

    struct addrinfo hints = {0};
    hints.ai_family       = AF_INET;
    hints.ai_socktype     = MapTransport(transport);

    struct addrinfo* info     = NULL;
    bool             resolved = false;

    if (getaddrinfo(host, NULL, &hints, &info) == GETADDRINFO_SUCCESS)
    {
        struct sockaddr_in* sin = SolidSyslogAddress_AsSockaddrIn(result);
        *sin                    = *(struct sockaddr_in*) info->ai_addr;
        sin->sin_port           = htons(port);
        freeaddrinfo(info);
        resolved = true;
    }

    return resolved;
}

static int MapTransport(enum SolidSyslogTransport transport)
{
    int socktype = SOCK_DGRAM;

    if (transport == SOLIDSYSLOG_TRANSPORT_TCP)
    {
        socktype = SOCK_STREAM;
    }

    return socktype;
}
