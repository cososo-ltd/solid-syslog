/* The at-rest policies over the OpenSSL pack. */
#include "BddTargetSecurityPolicy.h"

#include <stddef.h>

#include "BddTargetKey.h"
#include "BddTargetStoreSettings.h"
#include "SolidSyslogCrc16Policy.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogOpenSslAesGcmPolicy.h"
#include "SolidSyslogOpenSslHmacSha256Policy.h"

/* Latched at create so destroy answers what was built, not whatever
   `set security-policy` may name afterwards. */
static enum BddTargetSecurityPolicy installed;

struct SolidSyslogSecurityPolicy* BddTargetSecurityPolicy_Create(void)
{
    struct SolidSyslogSecurityPolicy* policy = NULL;
    installed = BddTargetStoreSettings_SecurityPolicy();
    if (installed == BDD_TARGET_SECURITY_POLICY_HMAC_SHA256)
    {
        static const struct SolidSyslogOpenSslHmacSha256PolicyConfig hmacConfig = {BddTargetKey_Get, NULL};
        policy = SolidSyslogOpenSslHmacSha256Policy_Create(&hmacConfig);
    }
    else if (installed == BDD_TARGET_SECURITY_POLICY_AES_256_GCM)
    {
        static const struct SolidSyslogOpenSslAesGcmPolicyConfig aesConfig = {BddTargetKey_Get, NULL};
        policy = SolidSyslogOpenSslAesGcmPolicy_Create(&aesConfig);
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
        SolidSyslogOpenSslHmacSha256Policy_Destroy(policy);
    }
    else if (installed == BDD_TARGET_SECURITY_POLICY_AES_256_GCM)
    {
        SolidSyslogOpenSslAesGcmPolicy_Destroy(policy);
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
