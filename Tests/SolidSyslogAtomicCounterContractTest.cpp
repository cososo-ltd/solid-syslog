#include "CppUTest/TestHarness.h"

#include <stdint.h>

extern "C"
{
#include "SolidSyslogAtomicCounterTestHelper.h"
}

TEST_GROUP(AtomicCounterContract){};

TEST(AtomicCounterContract, CreateReturnsNonNullHandle)
{
    TestAtomicCounterStorage storage;
    struct SolidSyslogAtomicCounter* counter = TestAtomicCounter_Create(&storage);

    CHECK(counter != nullptr);

    TestAtomicCounter_Destroy(counter);
}

TEST(AtomicCounterContract, FirstIncrementReturnsOne)
{
    TestAtomicCounterStorage storage;
    struct SolidSyslogAtomicCounter* counter = TestAtomicCounter_Create(&storage);

    LONGS_EQUAL(1, TestAtomicCounter_Increment(counter));

    TestAtomicCounter_Destroy(counter);
}

TEST(AtomicCounterContract, SequentialIncrementsCount1Then2Then3)
{
    TestAtomicCounterStorage storage;
    struct SolidSyslogAtomicCounter* counter = TestAtomicCounter_Create(&storage);

    LONGS_EQUAL(1, TestAtomicCounter_Increment(counter));
    LONGS_EQUAL(2, TestAtomicCounter_Increment(counter));
    LONGS_EQUAL(3, TestAtomicCounter_Increment(counter));

    TestAtomicCounter_Destroy(counter);
}

TEST(AtomicCounterContract, TwoCountersInSeparateStorageAreIndependent)
{
    TestAtomicCounterStorage storageA;
    TestAtomicCounterStorage storageB;
    struct SolidSyslogAtomicCounter* counterA = TestAtomicCounter_Create(&storageA);
    struct SolidSyslogAtomicCounter* counterB = TestAtomicCounter_Create(&storageB);

    LONGS_EQUAL(1, TestAtomicCounter_Increment(counterA));
    LONGS_EQUAL(2, TestAtomicCounter_Increment(counterA));
    LONGS_EQUAL(1, TestAtomicCounter_Increment(counterB));
    LONGS_EQUAL(3, TestAtomicCounter_Increment(counterA));
    LONGS_EQUAL(2, TestAtomicCounter_Increment(counterB));

    TestAtomicCounter_Destroy(counterA);
    TestAtomicCounter_Destroy(counterB);
}

TEST(AtomicCounterContract, IncrementAfterInitReturnsValuePlusOne)
{
    TestAtomicCounterStorage storage;
    struct SolidSyslogAtomicCounter* counter = TestAtomicCounter_Create(&storage);

    TestAtomicCounter_Init(counter, 5U);

    LONGS_EQUAL(6, TestAtomicCounter_Increment(counter));

    TestAtomicCounter_Destroy(counter);
}

TEST(AtomicCounterContract, IncrementAtMaxWrapsToOne)
{
    TestAtomicCounterStorage storage;
    struct SolidSyslogAtomicCounter* counter = TestAtomicCounter_Create(&storage);

    /* RFC 5424 §7.3.1: sequenceId values are in [1, 2^31 - 1] and wrap to 1, not 0. */
    TestAtomicCounter_Init(counter, (uint32_t) INT32_MAX);

    LONGS_EQUAL(1, TestAtomicCounter_Increment(counter));

    TestAtomicCounter_Destroy(counter);
}
