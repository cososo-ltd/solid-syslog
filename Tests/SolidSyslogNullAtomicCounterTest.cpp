#include "CppUTest/TestHarness.h"
#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogNullAtomicCounter.h"

// clang-format off
TEST_GROUP(SolidSyslogNullAtomicCounter)
{
    struct SolidSyslogAtomicCounter* counter = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        counter = SolidSyslogNullAtomicCounter_Get();
    }
};

// clang-format on

TEST(SolidSyslogNullAtomicCounter, IncrementReturnsOneToAvoidZero)
{
    LONGS_EQUAL(1U, SolidSyslogAtomicCounter_Increment(counter));
}

TEST(SolidSyslogNullAtomicCounter, IncrementIsIdempotent)
{
    SolidSyslogAtomicCounter_Increment(counter);
    LONGS_EQUAL(1U, SolidSyslogAtomicCounter_Increment(counter));
}
