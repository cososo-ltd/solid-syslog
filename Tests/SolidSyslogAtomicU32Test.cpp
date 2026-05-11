#include "SolidSyslogAtomicU32.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(SolidSyslogAtomicU32)
{
    SolidSyslogAtomicU32Storage  storage;
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    struct SolidSyslogAtomicU32* slot;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        slot = SolidSyslogAtomicU32_FromStorage(&storage);
        SolidSyslogAtomicU32_Init(slot, 0);
    }
};

// clang-format on

TEST(SolidSyslogAtomicU32, FromStorageReturnsNonNull)
{
    CHECK(slot != nullptr);
}

TEST(SolidSyslogAtomicU32, LoadAfterInitWithZeroReturnsZero)
{
    LONGS_EQUAL(0, SolidSyslogAtomicU32_Load(slot));
}

TEST(SolidSyslogAtomicU32, LoadAfterInitWithNonZeroReturnsInitValue)
{
    SolidSyslogAtomicU32_Init(slot, 42);
    LONGS_EQUAL(42, SolidSyslogAtomicU32_Load(slot));
}

TEST(SolidSyslogAtomicU32, LoadReturnsValueCommittedByCompareAndSwap)
{
    CHECK_TRUE(SolidSyslogAtomicU32_CompareAndSwap(slot, 0, 42));
    LONGS_EQUAL(42, SolidSyslogAtomicU32_Load(slot));
}

TEST(SolidSyslogAtomicU32, CompareAndSwapWithMismatchedExpectedReturnsFalseAndLeavesValueUnchanged)
{
    CHECK_FALSE(SolidSyslogAtomicU32_CompareAndSwap(slot, 99, 42));
    LONGS_EQUAL(0, SolidSyslogAtomicU32_Load(slot));
}
