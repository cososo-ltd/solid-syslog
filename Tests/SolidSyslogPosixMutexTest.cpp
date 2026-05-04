#include "CppUTest/TestHarness.h"
#include "SolidSyslogMutex.h"
#include "SolidSyslogPosixMutex.h"

// clang-format off
TEST_GROUP(SolidSyslogPosixMutex)
{
    SolidSyslogPosixMutexStorage storage{};
    struct SolidSyslogMutex*     mutex = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        mutex = SolidSyslogPosixMutex_Create(&storage);
    }

    void teardown() override
    {
        SolidSyslogPosixMutex_Destroy(mutex);
    }
};

// clang-format on

TEST(SolidSyslogPosixMutex, CreateDestroyDoesNotCrash)
{
}

TEST(SolidSyslogPosixMutex, HandleEqualsStorageAddress)
{
    POINTERS_EQUAL(&storage, mutex);
}

TEST(SolidSyslogPosixMutex, LockUnlockDoesNotCrash)
{
    SolidSyslogMutex_Lock(mutex);
    SolidSyslogMutex_Unlock(mutex);
}
