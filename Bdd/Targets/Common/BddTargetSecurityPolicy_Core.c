/* The at-rest policies with no crypto pack linked: CRC-16 and Null only. A
   keyed kind cannot be built, so it falls back to CRC-16 and says so on
   stderr, where the harness reads the target's errors; a NULL policy would
   be taken by the store as the Null policy, silently. */
#include "BddTargetSecurityPolicy.h"

#include <stddef.h>
#include <stdio.h>

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
    else
    {
        if (installed != BDD_TARGET_SECURITY_POLICY_CRC16)
        {
            fputs(
                "security-policy: keyed policies need a crypto pack this build does not carry; using crc16\n",
                stderr
            );
            installed = BDD_TARGET_SECURITY_POLICY_CRC16;
        }
        policy = SolidSyslogCrc16Policy_Create();
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
