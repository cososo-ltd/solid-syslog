#include "SolidSyslogPlusTcpResolver.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "FreeRTOS_DNS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

#include "SolidSyslogPlusTcpAddressPrivate.h"
#include "SolidSyslogPlusTcpResolverPrivate.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTransport.h"

struct SolidSyslogAddress;

enum
{
    GETADDRINFO_SUCCESS = 0
};

static bool PlusTcpResolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
);
static BaseType_t PlusTcpResolver_MapTransport(enum SolidSyslogTransport transport);

void PlusTcpResolver_Initialise(struct SolidSyslogResolver* base)
{
    base->Resolve = PlusTcpResolver_Resolve;
}

void PlusTcpResolver_Cleanup(struct SolidSyslogResolver* base)
{
    /* Overwrite the abstract base with the shared NullResolver vtable so
     * use-after-destroy resolves cleanly to a failed-lookup error path
     * rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullResolver_Get();
}

static bool PlusTcpResolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
)
{
    (void) base;

    struct freertos_addrinfo hints = {0};
    hints.ai_family = FREERTOS_AF_INET4;
    hints.ai_socktype = PlusTcpResolver_MapTransport(transport);

    struct freertos_addrinfo* info = NULL;
    bool resolved = false;

    if (FreeRTOS_getaddrinfo(host, NULL, &hints, &info) == GETADDRINFO_SUCCESS)
    {
        struct freertos_sockaddr* sockaddr = SolidSyslogPlusTcpAddress_AsFreertosSockaddr(result);
        sockaddr->sin_family = FREERTOS_AF_INET;
        sockaddr->sin_port = (uint16_t) FreeRTOS_htons(port);
        sockaddr->sin_address.ulIP_IPv4 = info->ai_addr->sin_address.ulIP_IPv4;
        FreeRTOS_freeaddrinfo(info);
        resolved = true;
    }

    return resolved;
}

static BaseType_t PlusTcpResolver_MapTransport(enum SolidSyslogTransport transport)
{
    BaseType_t socktype = FREERTOS_SOCK_DGRAM;

    if (transport == SOLIDSYSLOG_TRANSPORT_TCP)
    {
        socktype = FREERTOS_SOCK_STREAM;
    }

    return socktype;
}
