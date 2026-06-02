#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "FreeRTOS.h"
#include "FreeRtosSemaphoreFake.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFreeRtosMutex.h"
#include "SolidSyslogFreeRtosMutexErrors.h"
#include "SolidSyslogMutex.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

// Asserts handle is non-null and not one of the slots in pool.
#define CHECK_IS_FALLBACK(handle, pool)                                                \
    do                                                                                 \
    {                                                                                  \
        CHECK_TEXT((handle) != nullptr, "Fallback handle was nullptr");                \
        for (auto* slot : (pool))                                                      \
        {                                                                              \
            CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");   \
            CHECK_TEXT((handle) != slot, "Fallback handle collided with a pool slot"); \
        }                                                                              \
    } while (0)

// clang-format off
TEST_GROUP(SolidSyslogFreeRtosMutex)
{
    struct SolidSyslogMutex* mutex = nullptr;

    void setup() override
    {
        FreeRtosSemaphoreFake_Reset();
        mutex = SolidSyslogFreeRtosMutex_Create();
    }

    void teardown() override
    {
        SolidSyslogFreeRtosMutex_Destroy(mutex);
    }
};

// clang-format on

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
    mutex = nullptr;

    CALLED_FAKE(FreeRtosSemaphoreFake_SemaphoreDelete, ONCE);
}

// clang-format off
TEST_GROUP(SolidSyslogFreeRtosMutexPool)
{
    struct SolidSyslogMutex* pooled[SOLIDSYSLOG_MUTEX_POOL_SIZE] = {};
    struct SolidSyslogMutex* overflow                                      = nullptr;

    void setup() override
    {
        FreeRtosSemaphoreFake_Reset();
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogFreeRtosMutex_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogFreeRtosMutex_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogFreeRtosMutex_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogFreeRtosMutexPool, FillingPoolThenOverflowReturnsDistinctFallback)

{
    FillPool();

    overflow = SolidSyslogFreeRtosMutex_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogFreeRtosMutexPool, ExhaustedCreateReportsError)

{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogFreeRtosMutex_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&FreeRtosMutexErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_POOL_EXHAUSTED, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(FREERTOSMUTEX_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogFreeRtosMutexPool, FallbackLockUnlockAreNoOps)

{
    FillPool();
    FreeRtosSemaphoreFake_Reset();
    overflow = SolidSyslogFreeRtosMutex_Create();

    SolidSyslogMutex_Lock(overflow);
    SolidSyslogMutex_Unlock(overflow);

    CALLED_FAKE(FreeRtosSemaphoreFake_SemaphoreTake, NEVER);
    CALLED_FAKE(FreeRtosSemaphoreFake_SemaphoreGive, NEVER);
}

TEST(SolidSyslogFreeRtosMutexPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)

{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogFreeRtosMutex_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogFreeRtosMutexPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)

{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogFreeRtosMutex_Create();

    LONGS_EQUAL(SOLIDSYSLOG_MUTEX_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_MUTEX_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogFreeRtosMutexPool, DestroyOfPooledHandleLocksOnce)

{
    pooled[0] = SolidSyslogFreeRtosMutex_Create();
    ConfigLockFake_Install();

    SolidSyslogFreeRtosMutex_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogFreeRtosMutexPool, DestroyOfUnknownHandleDoesNotLock)

{
    ConfigLockFake_Install();
    struct SolidSyslogMutex stranger = {};

    SolidSyslogFreeRtosMutex_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogFreeRtosMutexPool, DestroyOfUnknownHandleReportsWarning)

{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogMutex stranger = {};

    SolidSyslogFreeRtosMutex_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&FreeRtosMutexErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_UNKNOWN_DESTROY, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(FREERTOSMUTEX_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogFreeRtosMutexPool, DestroyOfStaleHandleReportsWarning)

{
    pooled[0] = SolidSyslogFreeRtosMutex_Create();
    SolidSyslogFreeRtosMutex_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogFreeRtosMutex_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&FreeRtosMutexErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_UNKNOWN_DESTROY, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(FREERTOSMUTEX_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastDetail());
}
