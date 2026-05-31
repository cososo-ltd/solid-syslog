#include "SolidSyslogNullSecurityPolicy.h"

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogSecurityPolicyDefinition.h"

static bool NullSecurityPolicy_NullComputeIntegrity(
    struct SolidSyslogSecurityPolicy* self,
    const uint8_t* data,
    uint16_t length,
    // NOLINTNEXTLINE(readability-non-const-parameter) -- integrityOut is non-const to match the vtable signature
    uint8_t* integrityOut
)
{
    (void) self;
    (void) data;
    (void) length;
    (void) integrityOut;
    return true;
}

static bool NullSecurityPolicy_NullVerifyIntegrity(
    struct SolidSyslogSecurityPolicy* self,
    const uint8_t* data,
    uint16_t length,
    const uint8_t* integrityIn
)
{
    (void) self;
    (void) data;
    (void) length;
    (void) integrityIn;
    return true;
}

struct SolidSyslogSecurityPolicy* SolidSyslogNullSecurityPolicy_Get(void)
{
    static struct SolidSyslogSecurityPolicy instance = {
        0,
        NullSecurityPolicy_NullComputeIntegrity,
        NullSecurityPolicy_NullVerifyIntegrity,
    };
    return &instance;
}
