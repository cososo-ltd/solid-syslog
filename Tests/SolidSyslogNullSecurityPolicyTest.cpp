#include "CppUTest/TestHarness.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogSecurityPolicyDefinition.h"

// clang-format off
TEST_GROUP(SolidSyslogNullSecurityPolicy)
{
    struct SolidSyslogSecurityPolicy* policy = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        policy = SolidSyslogNullSecurityPolicy_Create();
    }

    void teardown() override
    {
        SolidSyslogNullSecurityPolicy_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogNullSecurityPolicy, CreateReturnsNonNull)
{
    CHECK_TRUE(policy != nullptr);
}

TEST(SolidSyslogNullSecurityPolicy, IntegritySizeIsZero)
{
    LONGS_EQUAL(0, policy->IntegritySize);
}

TEST(SolidSyslogNullSecurityPolicy, VerifyIntegrityReturnsTrue)
{
    CHECK_TRUE(policy->VerifyIntegrity(nullptr, 0, nullptr));
}

TEST(SolidSyslogNullSecurityPolicy, DestroyDoesNotCrash)
{
    SolidSyslogNullSecurityPolicy_Destroy();
}
