#include "CppUTest/TestHarness.h"
#include "SolidSyslogMutex.h"
#include "SolidSyslogNullMutex.h"

// clang-format off
TEST_GROUP(SolidSyslogNullMutex)
{
    struct SolidSyslogMutex* mutex = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        mutex = SolidSyslogNullMutex_Get();
    }
};

// clang-format on

TEST(SolidSyslogNullMutex, GetReturnsSameInstance)
{
    POINTERS_EQUAL(mutex, SolidSyslogNullMutex_Get());
}

TEST(SolidSyslogNullMutex, LockUnlockDoesNotCrash)
{
    SolidSyslogMutex_Lock(mutex);
    SolidSyslogMutex_Unlock(mutex);
}
