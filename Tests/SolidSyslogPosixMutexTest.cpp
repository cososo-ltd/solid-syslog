#include "CppUTest/TestHarness.h"

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "ErrorHandlerFakeEx.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogMutex.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogPosixMutex.h"
#include "SolidSyslogPosixMutexErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "TestUtils.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings ONCE/NEVER into scope for CALLED_FAKE

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while) -- macros preserve __FILE__/__LINE__ at the call site

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

// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

// clang-format off
TEST_GROUP(SolidSyslogPosixMutex)
{
    struct SolidSyslogMutex* mutex = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        mutex = SolidSyslogPosixMutex_Create();
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

TEST(SolidSyslogPosixMutex, LockUnlockDoesNotCrash)
{
    SolidSyslogMutex_Lock(mutex);
    SolidSyslogMutex_Unlock(mutex);
}

// clang-format off
TEST_GROUP(SolidSyslogPosixMutexPool)
{
    // cppcheck-suppress constVariable -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
    struct SolidSyslogMutex* pooled[SOLIDSYSLOG_POSIX_MUTEX_POOL_SIZE] = {};
    struct SolidSyslogMutex* overflow                                  = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogPosixMutex_Destroy(handle);
            }
        }
        // cppcheck-suppress knownConditionTrueFalse -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
        if (overflow != nullptr)
        {
            SolidSyslogPosixMutex_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
        ErrorHandlerFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogPosixMutex_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogPosixMutexPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogPosixMutex_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogPosixMutexPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFakeEx_Install(nullptr);
    FillPool();

    overflow = SolidSyslogPosixMutex_Create();

    CALLED_FAKE(ErrorHandlerFakeEx_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFakeEx_LastSeverity());
    POINTERS_EQUAL(&PosixMutexErrorSource, ErrorHandlerFakeEx_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXMUTEX_ERROR_POOL_EXHAUSTED, ErrorHandlerFakeEx_LastCode());
}

TEST(SolidSyslogPosixMutexPool, FallbackLockUnlockAreNoOps)
{
    FillPool();
    overflow = SolidSyslogPosixMutex_Create();

    SolidSyslogMutex_Lock(overflow);
    SolidSyslogMutex_Unlock(overflow);
}

TEST(SolidSyslogPosixMutexPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogPosixMutex_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPosixMutexPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogPosixMutex_Create();

    LONGS_EQUAL(SOLIDSYSLOG_POSIX_MUTEX_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_POSIX_MUTEX_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogPosixMutexPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogPosixMutex_Create();
    ConfigLockFake_Install();

    SolidSyslogPosixMutex_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPosixMutexPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogMutex stranger = {};

    SolidSyslogPosixMutex_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogPosixMutexPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFakeEx_Install(nullptr);
    struct SolidSyslogMutex stranger = {};

    SolidSyslogPosixMutex_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFakeEx_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFakeEx_LastSeverity());
    POINTERS_EQUAL(&PosixMutexErrorSource, ErrorHandlerFakeEx_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXMUTEX_ERROR_UNKNOWN_DESTROY, ErrorHandlerFakeEx_LastCode());
}

TEST(SolidSyslogPosixMutexPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogPosixMutex_Create();
    SolidSyslogPosixMutex_Destroy(pooled[0]);
    ErrorHandlerFakeEx_Install(nullptr);

    SolidSyslogPosixMutex_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFakeEx_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFakeEx_LastSeverity());
    POINTERS_EQUAL(&PosixMutexErrorSource, ErrorHandlerFakeEx_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXMUTEX_ERROR_UNKNOWN_DESTROY, ErrorHandlerFakeEx_LastCode());
}
