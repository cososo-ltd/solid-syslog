#include <cstdint>

#include "CppUTest/TestHarness.h"

#include "ErrorHandlerFake.h"
#include "SecurityPolicyFake.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyCategories.h"
#include "SolidSyslogSecurityPolicyDefinition.h"
#include "TestUtils.h"

using namespace CososoTesting;

// clang-format off
TEST_GROUP(SecurityPolicyFake)
{
    struct SolidSyslogSecurityPolicy* policy = nullptr;
    uint8_t content[8] = {};
    struct SolidSyslogSecurityRecord record = {content, sizeof(content), 2U, nullptr};

    void setup() override
    {
        policy = SecurityPolicyFake_Create();
        ErrorHandlerFake_Install(nullptr);
    }

    void teardown() override
    {
        SolidSyslog_SetErrorHandler(nullptr, nullptr);
        SecurityPolicyFake_Destroy(policy);
    }

    [[nodiscard]] bool Seal() const
    {
        return policy->SealRecord(policy, &record);
    }

    [[nodiscard]] enum SolidSyslogSecurityPolicyOpenResult Open() const
    {
        return policy->OpenRecord(policy, &record);
    }
};

// clang-format on

TEST(SecurityPolicyFake, AWorkingPolicySealsARecord)
{
    CHECK_TRUE(Seal());
}

TEST(SecurityPolicyFake, AWorkingPolicyOpensARecord)
{
    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_OPENED, Open());
}

TEST(SecurityPolicyFake, AWorkingPolicyReportsNothing)
{
    (void) Seal();
    (void) Open();

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SecurityPolicyFake, AWorkingPolicyAddsNoTrailer)
{
    LONGS_EQUAL(0, policy->TrailerSize);
}

TEST(SecurityPolicyFake, WithTheKeyWithheldASealFails)
{
    SecurityPolicyFake_SetFault(policy, SECURITY_POLICY_FAKE_KEY_WITHHELD);

    CHECK_FALSE(Seal());
}

TEST(SecurityPolicyFake, WithTheKeyWithheldAnOpenCannotRun)
{
    SecurityPolicyFake_SetFault(policy, SECURITY_POLICY_FAKE_KEY_WITHHELD);

    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_COULD_NOT_RUN, Open());
}

TEST(SecurityPolicyFake, AWithheldKeyIsReportedUnavailable)
{
    SecurityPolicyFake_SetFault(policy, SECURITY_POLICY_FAKE_KEY_WITHHELD);

    (void) Seal();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &SecurityPolicyFakeErrorSource,
        SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE,
        SECURITY_POLICY_FAKE_ERROR_KEY_UNAVAILABLE
    );
}

TEST(SecurityPolicyFake, WithItsBackendFailingASealFails)
{
    SecurityPolicyFake_SetFault(policy, SECURITY_POLICY_FAKE_BACKEND_FAILING);

    CHECK_FALSE(Seal());
}

TEST(SecurityPolicyFake, ASealTheBackendFailedIsReportedWithTheBackendsCode)
{
    SecurityPolicyFake_SetFault(policy, SECURITY_POLICY_FAKE_BACKEND_FAILING);

    (void) Seal();

    CHECK_ERROR_REPORTED_ONCE_WITH_NATIVE_CODE(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &SecurityPolicyFakeErrorSource,
        SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED,
        SECURITY_POLICY_FAKE_ERROR_BACKEND_FAILED,
        SECURITY_POLICY_FAKE_BACKEND_CODE
    );
}

TEST(SecurityPolicyFake, WithItsBackendFailingAnOpenCannotRun)
{
    SecurityPolicyFake_SetFault(policy, SECURITY_POLICY_FAKE_BACKEND_FAILING);

    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_COULD_NOT_RUN, Open());
}

TEST(SecurityPolicyFake, AnOpenTheBackendFailedIsReportedWithTheBackendsCode)
{
    SecurityPolicyFake_SetFault(policy, SECURITY_POLICY_FAKE_BACKEND_FAILING);

    (void) Open();

    CHECK_ERROR_REPORTED_ONCE_WITH_NATIVE_CODE(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &SecurityPolicyFakeErrorSource,
        SOLIDSYSLOG_CAT_SECURITY_POLICY_OPEN_FAILED,
        SECURITY_POLICY_FAKE_ERROR_BACKEND_FAILED,
        SECURITY_POLICY_FAKE_BACKEND_CODE
    );
}

TEST(SecurityPolicyFake, ARejectingPolicyRejectsAnOpen)
{
    SecurityPolicyFake_SetFault(policy, SECURITY_POLICY_FAKE_REJECTING);

    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_REJECTED, Open());
}

TEST(SecurityPolicyFake, ARejectedOpenIsAVerdictThePolicyDoesNotReport)
{
    SecurityPolicyFake_SetFault(policy, SECURITY_POLICY_FAKE_REJECTING);

    (void) Open();

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SecurityPolicyFake, ARejectingPolicyStillSeals)
{
    SecurityPolicyFake_SetFault(policy, SECURITY_POLICY_FAKE_REJECTING);

    CHECK_TRUE(Seal());
}

TEST(SecurityPolicyFake, ARunOfFaultsIsReportedOnceSealAndOpenTogether)
{
    SecurityPolicyFake_SetFault(policy, SECURITY_POLICY_FAKE_KEY_WITHHELD);
    (void) Seal();
    (void) Open();

    (void) Seal();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
}

TEST(SecurityPolicyFake, ASealThatReachesAVerdictEndsTheRun)
{
    SecurityPolicyFake_SetFault(policy, SECURITY_POLICY_FAKE_KEY_WITHHELD);
    (void) Seal();
    SecurityPolicyFake_SetFault(policy, SECURITY_POLICY_FAKE_WORKING);
    (void) Seal();
    SecurityPolicyFake_SetFault(policy, SECURITY_POLICY_FAKE_KEY_WITHHELD);

    (void) Seal();

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
}

TEST(SecurityPolicyFake, ARejectedOpenIsAVerdictThatEndsTheRun)
{
    SecurityPolicyFake_SetFault(policy, SECURITY_POLICY_FAKE_KEY_WITHHELD);
    (void) Open();
    SecurityPolicyFake_SetFault(policy, SECURITY_POLICY_FAKE_REJECTING);
    (void) Open();
    SecurityPolicyFake_SetFault(policy, SECURITY_POLICY_FAKE_KEY_WITHHELD);

    (void) Open();

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
}

TEST(SecurityPolicyFake, TheSourceNamesItsDetails)
{
    STRCMP_EQUAL("SecurityPolicyFake", SecurityPolicyFakeErrorSource.Name);
    CHECK_VOCABULARY_NAMED(SecurityPolicyFakeErrorSource.DetailName, SECURITY_POLICY_FAKE_ERROR_MAX);
}
