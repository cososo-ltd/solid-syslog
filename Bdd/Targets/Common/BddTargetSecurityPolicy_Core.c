/* The at-rest policies with no crypto pack linked: CRC-16 and Null only. A
   keyed kind cannot be built, so it is answered with no policy, which the
   store refuses and reports as bad configuration. */
#include "BddTargetSecurityPolicy.h"

#include <stddef.h>

#include "BddTargetStoreSettings.h"
#include "SolidSyslogCrc16Policy.h"
#include "SolidSyslogNullSecurityPolicy.h"

/* Latched at create so destroy answers what was built, not whatever
   `set security-policy` may name afterwards. */
static enum BddTargetSecurityPolicy installed;

struct SolidSyslogSecurityPolicy* BddTargetSecurityPolicy_Create(void)
{
    struct SolidSyslogSecurityPolicy* policy = NULL;
    installed = BddTargetStoreSettings_SecurityPolicy();
    if (installed == BDD_TARGET_SECURITY_POLICY_NULL)
    {
        policy = SolidSyslogNullSecurityPolicy_Get();
    }
    else if (installed == BDD_TARGET_SECURITY_POLICY_CRC16)
    {
        policy = SolidSyslogCrc16Policy_Create();
    }
    else
    {
        /* A keyed policy needs a crypto pack this build does not carry. */
    }
    return policy;
}

void BddTargetSecurityPolicy_Destroy(struct SolidSyslogSecurityPolicy* policy)
{
    (void) policy;
    if (installed == BDD_TARGET_SECURITY_POLICY_CRC16)
    {
        SolidSyslogCrc16Policy_Destroy();
    }
}
