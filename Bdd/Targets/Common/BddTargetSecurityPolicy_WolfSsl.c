/* The at-rest policies over the wolfSSL pack. The AES-GCM policy draws its
   nonces from a wolfCrypt RNG the target owns, initialised on first use. */
#include "BddTargetSecurityPolicy.h"

#ifndef WOLFSSL_USER_SETTINGS
#include <wolfssl/options.h>
#endif
#include <stddef.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/settings.h>

#include "BddTargetKey.h"
#include "BddTargetStoreSettings.h"
#include "SolidSyslogCrc16Policy.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogWolfSslAesGcmPolicy.h"
#include "SolidSyslogWolfSslHmacSha256Policy.h"

/* Latched at create so destroy answers what was built, not whatever
   `set security-policy` may name afterwards. */
static enum BddTargetSecurityPolicy installed;
static WC_RNG rng;

struct SolidSyslogSecurityPolicy* BddTargetSecurityPolicy_Create(void)
{
    struct SolidSyslogSecurityPolicy* policy = NULL;
    installed = BddTargetStoreSettings_SecurityPolicy();
    if (installed == BDD_TARGET_SECURITY_POLICY_HMAC_SHA256)
    {
        static const struct SolidSyslogWolfSslHmacSha256PolicyConfig hmacConfig = {BddTargetKey_Get, NULL};
        policy = SolidSyslogWolfSslHmacSha256Policy_Create(&hmacConfig);
    }
    else if (installed == BDD_TARGET_SECURITY_POLICY_AES_256_GCM)
    {
        /* A seed failure leaves Rng NULL, which Create refuses as bad config
           and reports, so the harness sees why rather than an unseeded RNG. */
        struct SolidSyslogWolfSslAesGcmPolicyConfig aesConfig = {BddTargetKey_Get, NULL, NULL};
        if (wc_InitRng(&rng) == 0)
        {
            aesConfig.Rng = &rng;
        }
        policy = SolidSyslogWolfSslAesGcmPolicy_Create(&aesConfig);
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
        SolidSyslogWolfSslHmacSha256Policy_Destroy(policy);
    }
    else if (installed == BDD_TARGET_SECURITY_POLICY_AES_256_GCM)
    {
        SolidSyslogWolfSslAesGcmPolicy_Destroy(policy);
        wc_FreeRng(&rng);
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
