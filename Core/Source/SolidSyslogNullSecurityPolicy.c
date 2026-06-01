#include "SolidSyslogNullSecurityPolicy.h"

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogSecurityPolicyDefinition.h"

static bool NullSecurityPolicy_NullSealRecord(
    struct SolidSyslogSecurityPolicy* self,
    // NOLINTNEXTLINE(readability-non-const-parameter) -- content is non-const to match the vtable signature
    uint8_t* content,
    uint16_t contentLength,
    uint16_t headerLength,
    // NOLINTNEXTLINE(readability-non-const-parameter) -- trailerOut is non-const to match the vtable signature
    uint8_t* trailerOut
)
{
    (void) self;
    (void) content;
    (void) contentLength;
    (void) headerLength;
    (void) trailerOut;
    return true;
}

static bool NullSecurityPolicy_NullOpenRecord(
    struct SolidSyslogSecurityPolicy* self,
    // NOLINTNEXTLINE(readability-non-const-parameter) -- content is non-const to match the vtable signature
    uint8_t* content,
    uint16_t contentLength,
    uint16_t headerLength,
    const uint8_t* trailerIn
)
{
    (void) self;
    (void) content;
    (void) contentLength;
    (void) headerLength;
    (void) trailerIn;
    return true;
}

struct SolidSyslogSecurityPolicy* SolidSyslogNullSecurityPolicy_Get(void)
{
    static struct SolidSyslogSecurityPolicy instance = {
        0,
        NullSecurityPolicy_NullSealRecord,
        NullSecurityPolicy_NullOpenRecord,
    };
    return &instance;
}
