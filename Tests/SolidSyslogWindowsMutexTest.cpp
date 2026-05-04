#include "CppUTest/TestHarness.h"
#include "SolidSyslogMutex.h"
#include "SolidSyslogWindowsMutex.h"

// clang-format off
TEST_GROUP(SolidSyslogWindowsMutex)
{
    SolidSyslogWindowsMutexStorage storage{};
    struct SolidSyslogMutex*       mutex = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        mutex = SolidSyslogWindowsMutex_Create(&storage);
    }

    void teardown() override
    {
        SolidSyslogWindowsMutex_Destroy(mutex);
    }
};

// clang-format on

TEST(SolidSyslogWindowsMutex, CreateDestroyDoesNotCrash)
{
}

TEST(SolidSyslogWindowsMutex, HandleEqualsStorageAddress)
{
    POINTERS_EQUAL(&storage, mutex);
}

TEST(SolidSyslogWindowsMutex, LockUnlockDoesNotCrash)
{
    SolidSyslogMutex_Lock(mutex);
    SolidSyslogMutex_Unlock(mutex);
}
