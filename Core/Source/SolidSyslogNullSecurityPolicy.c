#include "SolidSyslogNullSecurityPolicy.h"

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogSecurityPolicyDefinition.h"

// NOLINTNEXTLINE(readability-non-const-parameter) -- matches SecurityPolicy vtable signature
static void NullComputeIntegrity(const uint8_t* data, uint16_t length, uint8_t* integrityOut)
{
    (void) data;
    (void) length;
    (void) integrityOut;
}

static bool NullVerifyIntegrity(const uint8_t* data, uint16_t length, const uint8_t* integrityIn)
{
    (void) data;
    (void) length;
    (void) integrityIn;
    return true;
}

static struct SolidSyslogSecurityPolicy instance = {
    0,
    NullComputeIntegrity,
    NullVerifyIntegrity,
};

struct SolidSyslogSecurityPolicy* SolidSyslogNullSecurityPolicy_Create(void)
{
    return &instance;
}

void SolidSyslogNullSecurityPolicy_Destroy(void)
{
}
