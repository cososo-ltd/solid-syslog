#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
                               // macros

#include "FreeRtosSemaphoreFake.h"
#include "SolidSyslogFreeRtosMutex.h"
#include "SolidSyslogMutex.h"

#include "FreeRTOS.h"
#include "semphr.h"

// clang-format off
TEST_GROUP(SolidSyslogFreeRtosMutex)
{
    SolidSyslogFreeRtosMutexStorage storage{};
    struct SolidSyslogMutex*        mutex = nullptr;

    void setup() override
    {
        FreeRtosSemaphoreFake_Reset();
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        mutex = SolidSyslogFreeRtosMutex_Create(&storage);
    }
};

// clang-format on

TEST(SolidSyslogFreeRtosMutex, HandleEqualsStorageAddress)
{
    POINTERS_EQUAL(&storage, mutex);
}

TEST(SolidSyslogFreeRtosMutex, CreateCallsCreateMutexStaticOnce)
{
    CALLED_FAKE(FreeRtosSemaphoreFake_CreateMutexStatic, ONCE);
}

TEST(SolidSyslogFreeRtosMutex, LockCallsSemaphoreTakeOnce)
{
    SolidSyslogMutex_Lock(mutex);

    CALLED_FAKE(FreeRtosSemaphoreFake_SemaphoreTake, ONCE);
}

TEST(SolidSyslogFreeRtosMutex, UnlockCallsSemaphoreGiveOnce)
{
    SolidSyslogMutex_Unlock(mutex);

    CALLED_FAKE(FreeRtosSemaphoreFake_SemaphoreGive, ONCE);
}

TEST(SolidSyslogFreeRtosMutex, DestroyCallsSemaphoreDeleteOnce)
{
    SolidSyslogFreeRtosMutex_Destroy(mutex);

    CALLED_FAKE(FreeRtosSemaphoreFake_SemaphoreDelete, ONCE);
}
