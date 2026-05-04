#include "SolidSyslogAtomicOpsDefinition.h"
#include "SolidSyslogStdAtomicOps.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(SolidSyslogStdAtomicOps)
{
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    SolidSyslogAtomicOps* ops;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        ops = SolidSyslogStdAtomicOps_Create();
    }

    void teardown() override
    {
        SolidSyslogStdAtomicOps_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogStdAtomicOps, CreateReturnsNonNull)
{
    CHECK(ops != nullptr);
}

TEST(SolidSyslogStdAtomicOps, LoadAfterCreateReturns0)
{
    LONGS_EQUAL(0, ops->Load(ops));
}

TEST(SolidSyslogStdAtomicOps, LoadReturnsValueCommittedByCompareAndSwap)
{
    CHECK_TRUE(ops->CompareAndSwap(ops, 0, 42));
    LONGS_EQUAL(42, ops->Load(ops));
}

TEST(SolidSyslogStdAtomicOps, CompareAndSwapWithMismatchedExpectedReturnsFalseAndLeavesValueUnchanged)
{
    CHECK_FALSE(ops->CompareAndSwap(ops, 99, 42));
    LONGS_EQUAL(0, ops->Load(ops));
}
