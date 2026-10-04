#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogAtomicCounterTestHelper.h"
#include "SolidSyslogVxWorks64AtomicCounter.h"
#include "VxWorks64IntFake.h"

static struct SolidSyslogAtomicCounter* hookedCounter = nullptr;

static void SetCounterTo41(void)
{
    TestAtomicCounter_Init(hookedCounter, 41U);
}

static void SetCounterTo99(void)
{
    TestAtomicCounter_Init(hookedCounter, 99U);
}

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64AtomicCounter)
{
    struct SolidSyslogAtomicCounter* counter = nullptr;

    void setup() override
    {
        VxWorks64IntFake_Reset();
        counter = SolidSyslogVxWorks64AtomicCounter_Create();
        hookedCounter = counter;
    }

    void teardown() override
    {
        SolidSyslogVxWorks64AtomicCounter_Destroy(counter);
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64AtomicCounter, IncrementLocksInterruptsOnce)
{
    (void) SolidSyslogAtomicCounter_Increment(counter);

    LONGS_EQUAL(1, VxWorks64IntFake_IntLockCallCount());
}

TEST(SolidSyslogVxWorks64AtomicCounter, IncrementUnlocksInterruptsOnce)
{
    (void) SolidSyslogAtomicCounter_Increment(counter);

    LONGS_EQUAL(1, VxWorks64IntFake_IntUnlockCallCount());
}

TEST(SolidSyslogVxWorks64AtomicCounter, IncrementUnlocksWithTheKeyIntLockReturned)
{
    VxWorks64IntFake_SetLockKey(42);

    (void) SolidSyslogAtomicCounter_Increment(counter);

    LONGS_EQUAL(42, VxWorks64IntFake_LastUnlockKey());
}

TEST(SolidSyslogVxWorks64AtomicCounter, IncrementReadsTheValueOnlyOnceInterruptsAreLocked)
{
    VxWorks64IntFake_SetLockHook(SetCounterTo41);

    LONGS_EQUAL(42, SolidSyslogAtomicCounter_Increment(counter));
}

TEST(SolidSyslogVxWorks64AtomicCounter, IncrementWritesTheValueBeforeInterruptsAreUnlocked)
{
    VxWorks64IntFake_SetUnlockHook(SetCounterTo99);
    (void) SolidSyslogAtomicCounter_Increment(counter);
    VxWorks64IntFake_SetUnlockHook(nullptr);

    LONGS_EQUAL(100, SolidSyslogAtomicCounter_Increment(counter));
}
