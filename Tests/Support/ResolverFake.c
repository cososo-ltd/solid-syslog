#include "ResolverFake.h"

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogResolverDefinition.h"

static int resolveCallCount;

static bool Resolve(
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
    (void) result;
    resolveCallCount++;
    return true;
}

struct SolidSyslogResolver* ResolverFake_Get(void)
{
    static struct SolidSyslogResolver instance = {Resolve};
    return &instance;
}

int ResolverFake_ResolveCallCount(void)
{
    return resolveCallCount;
}
