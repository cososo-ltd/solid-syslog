#include "SolidSyslogNullResolver.h"

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTransport.h"

struct SolidSyslogAddress;

static bool NullResolver_Resolve(
    struct SolidSyslogResolver* base,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
);

struct SolidSyslogResolver* SolidSyslogNullResolver_Get(void)
{
    static struct SolidSyslogResolver instance = {.Resolve = NullResolver_Resolve};
    return &instance;
}

/* Resolve returns false ("could not resolve") so callers' existing
 * unresolved-host error path runs naturally. In the pool-exhausted
 * fallback case the matching Sender is already on NullSender, so the
 * send chain is broken end-to-end. */
static bool NullResolver_Resolve(
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
    return false;
}
