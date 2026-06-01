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

TEST(SolidSyslogNullSecurityPolicy, TrailerSizeIsZero)
{
    LONGS_EQUAL(0, policy->TrailerSize);
}

TEST(SolidSyslogNullSecurityPolicy, OpenRecordReturnsTrue)
{
    CHECK_TRUE(policy->OpenRecord(policy, nullptr, 0, 0, nullptr));
}
