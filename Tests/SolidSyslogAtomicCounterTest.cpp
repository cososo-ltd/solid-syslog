#include "AtomicOpsFake.h"
#include "SolidSyslogAtomicCounter.h"
#include "TestAtomicOps.h"
#include "CppUTest/TestHarness.h"

struct SolidSyslogAtomicCounter;

enum
{
    SEQUENCE_ID_MAX            = 2147483647,
    SEQUENCE_ID_JUST_BELOW_MAX = 2147483646,
};

// clang-format off
TEST_GROUP(SolidSyslogAtomicCounter)
{
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    SolidSyslogAtomicCounter* counter;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        counter = SolidSyslogAtomicCounter_Create(TestAtomicOps_Create());
    }

    void teardown() override
    {
        SolidSyslogAtomicCounter_Destroy();
        TestAtomicOps_Destroy();
    }
};

TEST_GROUP(SolidSyslogAtomicCounterWithOps)
{
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    SolidSyslogAtomicCounter* counter;

    void setup() override
    {
        AtomicOpsFake_Reset();
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        counter = SolidSyslogAtomicCounter_Create(AtomicOpsFake_Get());
    }

    void teardown() override
    {
        SolidSyslogAtomicCounter_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogAtomicCounter, CreateReturnsNonNull)
{
    CHECK(counter != nullptr);
}

TEST(SolidSyslogAtomicCounter, FirstIncrementReturns1)
{
    LONGS_EQUAL(1, SolidSyslogAtomicCounter_Increment(counter));
}

TEST(SolidSyslogAtomicCounter, SecondIncrementReturns2)
{
    SolidSyslogAtomicCounter_Increment(counter);
    LONGS_EQUAL(2, SolidSyslogAtomicCounter_Increment(counter));
}

TEST(SolidSyslogAtomicCounter, ThirdIncrementReturns3)
{
    SolidSyslogAtomicCounter_Increment(counter);
    SolidSyslogAtomicCounter_Increment(counter);
    LONGS_EQUAL(3, SolidSyslogAtomicCounter_Increment(counter));
}

TEST(SolidSyslogAtomicCounterWithOps, IncrementReturnsLoadPlus1)
{
    AtomicOpsFake_SetLoadValue(41);
    LONGS_EQUAL(42, SolidSyslogAtomicCounter_Increment(counter));
}

TEST(SolidSyslogAtomicCounterWithOps, IncrementJustBelowMaxReturnsMax)
{
    AtomicOpsFake_SetLoadValue(SEQUENCE_ID_JUST_BELOW_MAX);
    LONGS_EQUAL(SEQUENCE_ID_MAX, SolidSyslogAtomicCounter_Increment(counter));
}

TEST(SolidSyslogAtomicCounterWithOps, IncrementWrapsFromRfcMaximumTo1)
{
    AtomicOpsFake_SetLoadValue(SEQUENCE_ID_MAX);
    LONGS_EQUAL(1, SolidSyslogAtomicCounter_Increment(counter));
}

TEST(SolidSyslogAtomicCounterWithOps, SecondIncrementSeesCommittedValue)
{
    AtomicOpsFake_SetLoadValue(7);
    SolidSyslogAtomicCounter_Increment(counter);
    LONGS_EQUAL(9, SolidSyslogAtomicCounter_Increment(counter));
}

TEST(SolidSyslogAtomicCounterWithOps, IncrementRetriesWhenCompareAndSwapFails)
{
    AtomicOpsFake_SetLoadValue(5);
    AtomicOpsFake_FailNextCompareAndSwapAndShiftLoadTo(6);
    LONGS_EQUAL(7, SolidSyslogAtomicCounter_Increment(counter));
}
