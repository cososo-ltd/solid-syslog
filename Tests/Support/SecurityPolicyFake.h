#ifndef SECURITYPOLICYFAKE_H
#define SECURITYPOLICYFAKE_H

#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;
    struct SolidSyslogSecurityPolicy;

    /* What the fake does with the next seal or open. Working, it seals and
     * opens as the Null policy does, with no trailer. Its faults are reported
     * as a keyed policy reports them: once for a run of them, seal and open
     * together, re-armed by the next seal or open that reaches a verdict. */
    enum SecurityPolicyFakeFault
    {
        SECURITY_POLICY_FAKE_WORKING,
        /* A seal fails and an open cannot run, each for want of the key. */
        SECURITY_POLICY_FAKE_KEY_WITHHELD,
        /* A seal fails and an open cannot run, each with the backend's code. */
        SECURITY_POLICY_FAKE_BACKEND_FAILING,
        /* Every record fails verification, a verdict the store reports. */
        SECURITY_POLICY_FAKE_REJECTING
    };

    /* The details it raises, under SECURITY_POLICY_KEY_UNAVAILABLE for the key
     * and under SEAL_FAILED or OPEN_FAILED for the backend. */
    enum SecurityPolicyFakeErrors
    {
        SECURITY_POLICY_FAKE_ERROR_KEY_UNAVAILABLE,
        SECURITY_POLICY_FAKE_ERROR_BACKEND_FAILED,
        SECURITY_POLICY_FAKE_ERROR_MAX
    };

    /* The native code a failing backend gives, as a crypto library would. */
    enum
    {
        SECURITY_POLICY_FAKE_BACKEND_CODE = 42
    };

    struct SolidSyslogSecurityPolicy* SecurityPolicyFake_Create(void);
    void SecurityPolicyFake_Destroy(struct SolidSyslogSecurityPolicy * policy);
    void SecurityPolicyFake_SetFault(struct SolidSyslogSecurityPolicy * policy, enum SecurityPolicyFakeFault fault);

    extern const struct SolidSyslogErrorSource SecurityPolicyFakeErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SECURITYPOLICYFAKE_H */
