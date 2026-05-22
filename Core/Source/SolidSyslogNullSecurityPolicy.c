#include "SolidSyslogNullSecurityPolicy.h"

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogSecurityPolicyDefinition.h"

// NOLINTNEXTLINE(readability-non-const-parameter) -- matches SecurityPolicy vtable signature
static void NullSecurityPolicy_NullComputeIntegrity(const uint8_t* data, uint16_t length, uint8_t* integrityOut)
{
    (void) data;
    (void) length;
    (void) integrityOut;
}

static bool NullSecurityPolicy_NullVerifyIntegrity(const uint8_t* data, uint16_t length, const uint8_t* integrityIn)
{
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
