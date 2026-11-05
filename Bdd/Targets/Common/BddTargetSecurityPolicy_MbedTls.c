/* The at-rest policies over the Mbed TLS pack. The AES-GCM policy draws its
   nonces from the CTR-DRBG the TLS sender already seeded, rather than standing
   up a second entropy and DRBG pair on a resource-constrained target. */
#include "BddTargetSecurityPolicy.h"

#include <stddef.h>

#include "BddTargetKey.h"
#include "BddTargetStoreSettings.h"
#include "BddTargetTlsSender.h"
#include "SolidSyslogCrc16Policy.h"
#include "SolidSyslogMbedTlsAesGcmPolicy.h"
#include "SolidSyslogMbedTlsHmacSha256Policy.h"
#include "SolidSyslogNullSecurityPolicy.h"

/* Latched at create so destroy answers what was built, not whatever
   `set security-policy` may name afterwards. */
static enum BddTargetSecurityPolicy installed = BDD_TARGET_SECURITY_POLICY_CRC16;

struct SolidSyslogSecurityPolicy* BddTargetSecurityPolicy_Create(void)
{
    struct SolidSyslogSecurityPolicy* policy = NULL;
    installed = BddTargetStoreSettings_SecurityPolicy();
    if (installed == BDD_TARGET_SECURITY_POLICY_HMAC_SHA256)
    {
        static const struct SolidSyslogMbedTlsHmacSha256PolicyConfig hmacConfig = {BddTargetKey_Get, NULL};
        policy = SolidSyslogMbedTlsHmacSha256Policy_Create(&hmacConfig);
    }
    else if (installed == BDD_TARGET_SECURITY_POLICY_AES_256_GCM)
    {
        /* Not static const: Rng is a runtime handle. */
        const struct SolidSyslogMbedTlsAesGcmPolicyConfig aesConfig =
            {BddTargetKey_Get, NULL, BddTargetTlsSender_GetRng()};
        policy = SolidSyslogMbedTlsAesGcmPolicy_Create(&aesConfig);
    }
    else if (installed == BDD_TARGET_SECURITY_POLICY_NULL)
    {
        policy = SolidSyslogNullSecurityPolicy_Get();
    }
    else
    {
        policy = SolidSyslogCrc16Policy_Create();
    }
    return policy;
}

void BddTargetSecurityPolicy_Destroy(struct SolidSyslogSecurityPolicy* policy)
{
    if (installed == BDD_TARGET_SECURITY_POLICY_HMAC_SHA256)
    {
        SolidSyslogMbedTlsHmacSha256Policy_Destroy(policy);
    }
    else if (installed == BDD_TARGET_SECURITY_POLICY_AES_256_GCM)
    {
        SolidSyslogMbedTlsAesGcmPolicy_Destroy(policy);
    }
    else if (installed == BDD_TARGET_SECURITY_POLICY_CRC16)
    {
        SolidSyslogCrc16Policy_Destroy();
    }
    else
    {
        /* The shared NullSecurityPolicy is immutable - nothing to free. */
    }
}
