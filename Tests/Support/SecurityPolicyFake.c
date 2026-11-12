#include "TestCalloc.h"
#include "SecurityPolicyFake.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyCategories.h"
#include "SolidSyslogSecurityPolicyDefinition.h"

struct SecurityPolicyFake
{
    struct SolidSyslogSecurityPolicy Base;
    enum SecurityPolicyFakeFault Fault;
    bool FaultReported;
};

static const char* DetailName(int32_t detail);
static bool SealRecord(struct SolidSyslogSecurityPolicy* base, const struct SolidSyslogSecurityRecord* record);
static enum SolidSyslogSecurityPolicyOpenResult OpenRecord(
    struct SolidSyslogSecurityPolicy* base,
    const struct SolidSyslogSecurityRecord* record
);
static void ReportFault(struct SecurityPolicyFake* fake, uint16_t category, enum SecurityPolicyFakeErrors detail);

const struct SolidSyslogErrorSource SecurityPolicyFakeErrorSource = {"SecurityPolicyFake", DetailName};

static const char* DetailName(int32_t detail)
{
    static const char* const names[SECURITY_POLICY_FAKE_ERROR_MAX] = {"KEY_UNAVAILABLE", "BACKEND_FAILED"};
    return ((detail >= 0) && (detail < SECURITY_POLICY_FAKE_ERROR_MAX)) ? names[detail] : NULL;
}

struct SolidSyslogSecurityPolicy* SecurityPolicyFake_Create(void)
{
    struct SecurityPolicyFake* fake = (struct SecurityPolicyFake*) TestCalloc(sizeof(struct SecurityPolicyFake));
    fake->Base.TrailerSize = 0U;
    fake->Base.SealRecord = SealRecord;
    fake->Base.OpenRecord = OpenRecord;
    fake->Fault = SECURITY_POLICY_FAKE_WORKING;
    return &fake->Base;
}

void SecurityPolicyFake_Destroy(struct SolidSyslogSecurityPolicy* policy)
{
    free(policy);
}

void SecurityPolicyFake_SetFault(struct SolidSyslogSecurityPolicy* policy, enum SecurityPolicyFakeFault fault)
{
    ((struct SecurityPolicyFake*) policy)->Fault = fault;
}

static bool SealRecord(struct SolidSyslogSecurityPolicy* base, const struct SolidSyslogSecurityRecord* record)
{
    struct SecurityPolicyFake* fake = (struct SecurityPolicyFake*) base;
    bool sealed = false;
    (void) record;

    if (fake->Fault == SECURITY_POLICY_FAKE_KEY_WITHHELD)
    {
        ReportFault(fake, SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE, SECURITY_POLICY_FAKE_ERROR_KEY_UNAVAILABLE);
    }
    else if (fake->Fault == SECURITY_POLICY_FAKE_BACKEND_FAILING)
    {
        ReportFault(fake, SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED, SECURITY_POLICY_FAKE_ERROR_BACKEND_FAILED);
    }
    else
    {
        fake->FaultReported = false;
        sealed = true;
    }

    return sealed;
}

static enum SolidSyslogSecurityPolicyOpenResult OpenRecord(
    struct SolidSyslogSecurityPolicy* base,
    const struct SolidSyslogSecurityRecord* record
)
{
    struct SecurityPolicyFake* fake = (struct SecurityPolicyFake*) base;
    enum SolidSyslogSecurityPolicyOpenResult result = SOLIDSYSLOG_SECURITY_POLICY_OPEN_COULD_NOT_RUN;
    (void) record;

    if (fake->Fault == SECURITY_POLICY_FAKE_KEY_WITHHELD)
    {
        ReportFault(fake, SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE, SECURITY_POLICY_FAKE_ERROR_KEY_UNAVAILABLE);
    }
    else if (fake->Fault == SECURITY_POLICY_FAKE_BACKEND_FAILING)
    {
        ReportFault(fake, SOLIDSYSLOG_CAT_SECURITY_POLICY_OPEN_FAILED, SECURITY_POLICY_FAKE_ERROR_BACKEND_FAILED);
    }
    else
    {
        fake->FaultReported = false;
        result = (fake->Fault == SECURITY_POLICY_FAKE_REJECTING) ? SOLIDSYSLOG_SECURITY_POLICY_OPEN_REJECTED
                                                                 : SOLIDSYSLOG_SECURITY_POLICY_OPEN_OPENED;
    }

    return result;
}

/* Marked before it is reported, as the packs do, so a handler that logs finds
 * the run already reported. A backend fault carries the backend's code; a
 * missing key has none. */
static void ReportFault(struct SecurityPolicyFake* fake, uint16_t category, enum SecurityPolicyFakeErrors detail)
{
    bool runStarted = !fake->FaultReported;
    fake->FaultReported = true;

    if (runStarted && (detail == SECURITY_POLICY_FAKE_ERROR_BACKEND_FAILED))
    {
        SolidSyslog_ErrorWithNativeCode(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &SecurityPolicyFakeErrorSource,
            category,
            (int32_t) detail,
            SECURITY_POLICY_FAKE_BACKEND_CODE
        );
    }
    else if (runStarted)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &SecurityPolicyFakeErrorSource, category, (int32_t) detail);
    }
    else
    {
        /* Still the run already reported. */
    }
}
