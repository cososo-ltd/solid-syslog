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
        mutex = SolidSyslogNullMutex_Create();
    }

    void teardown() override
    {
        SolidSyslogNullMutex_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogNullMutex, CreateDestroyDoesNotCrash)
{
}

TEST(SolidSyslogNullMutex, LockUnlockDoesNotCrash)
{
    SolidSyslogMutex_Lock(mutex);
    SolidSyslogMutex_Unlock(mutex);
}
