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

TEST(SolidSyslogNullSecurityPolicy, OpenRecordOpens)
{
    struct SolidSyslogSecurityRecord record = {nullptr, 0, 0, nullptr};
    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_OPENED, policy->OpenRecord(policy, &record));
}
