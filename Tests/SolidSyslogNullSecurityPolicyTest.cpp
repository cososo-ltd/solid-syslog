#include "CppUTest/TestHarness.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogSecurityPolicyDefinition.h"

// clang-format off
TEST_GROUP(SolidSyslogNullSecurityPolicy)
{
    struct SolidSyslogSecurityPolicy* policy = nullptr;

    void setup() override
    {
        policy = SolidSyslogNullSecurityPolicy_Get();
    }
};

// clang-format on

TEST(SolidSyslogNullSecurityPolicy, GetReturnsNonNull)
{
    CHECK_TRUE(policy != nullptr);
}

TEST(SolidSyslogNullSecurityPolicy, IntegritySizeIsZero)
{
    LONGS_EQUAL(0, policy->IntegritySize);
}

TEST(SolidSyslogNullSecurityPolicy, VerifyIntegrityReturnsTrue)
{
    CHECK_TRUE(policy->VerifyIntegrity(policy, nullptr, 0, nullptr));
}
