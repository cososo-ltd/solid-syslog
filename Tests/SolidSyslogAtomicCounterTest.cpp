/* Whitebox include: SolidSyslogAtomicCounter.c is compiled into this test
   translation unit so the static helpers NextSequenceId and TryAdvance are
   directly reachable. The library's own SolidSyslogAtomicCounter.o is then
   not pulled in by the linker (static-archive object-on-demand resolution),
   so there is no duplicate-symbol conflict. */
// NOLINTNEXTLINE(bugprone-suspicious-include)
#include "SolidSyslogAtomicCounter.c"

#include "SolidSyslogAtomicU32.h"
#include "CppUTest/TestHarness.h"

#include <stdint.h>

enum
{
    TEST_SEQUENCE_ID_MAX = 2147483647,
    TEST_SEQUENCE_ID_JUST_BELOW_MAX = 2147483646,
};

// clang-format off
TEST_GROUP(SolidSyslogAtomicCounter)
{
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    SolidSyslogAtomicCounter* counter;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        counter = SolidSyslogAtomicCounter_Create();
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

TEST(SolidSyslogAtomicCounter, NextSequenceIdAfterZeroReturns1)
{
    LONGS_EQUAL(1, AtomicCounter_NextSequenceId(0));
}

TEST(SolidSyslogAtomicCounter, NextSequenceIdJustBelowMaxReturnsMax)
{
    LONGS_EQUAL(TEST_SEQUENCE_ID_MAX, AtomicCounter_NextSequenceId(TEST_SEQUENCE_ID_JUST_BELOW_MAX));
}

TEST(SolidSyslogAtomicCounter, NextSequenceIdAtMaxWrapsTo1)
{
    LONGS_EQUAL(1, AtomicCounter_NextSequenceId(TEST_SEQUENCE_ID_MAX));
}

TEST(SolidSyslogAtomicCounter, TryAdvanceRereadsSlotAfterExternalMutation)
{
    uint32_t firstNext = 0;
    CHECK_TRUE(AtomicCounter_TryAdvance(counter->Slot, &firstNext));
    LONGS_EQUAL(1, firstNext);

    /* Simulate "another writer committed first" — TryAdvance must re-Load
       (rather than reuse a stale current) on its next call, so the returned
       value is one above the slot's actual current value. */
    SolidSyslogAtomicU32_Init(counter->Slot, 5);

    uint32_t secondNext = 0;
    CHECK_TRUE(AtomicCounter_TryAdvance(counter->Slot, &secondNext));
    LONGS_EQUAL(6, secondNext);
}
