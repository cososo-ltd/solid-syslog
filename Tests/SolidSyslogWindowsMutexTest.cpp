#include "CppUTest/TestHarness.h"

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogMutex.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWindowsMutex.h"
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
TEST_GROUP(SolidSyslogWindowsMutex)
{
    struct SolidSyslogMutex* mutex = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        mutex = SolidSyslogWindowsMutex_Create();
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

TEST(SolidSyslogWindowsMutex, LockUnlockDoesNotCrash)
{
    SolidSyslogMutex_Lock(mutex);
    SolidSyslogMutex_Unlock(mutex);
}

// clang-format off
TEST_GROUP(SolidSyslogWindowsMutexPool)
{
    // cppcheck-suppress constVariable -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
    struct SolidSyslogMutex* pooled[SOLIDSYSLOG_WINDOWS_MUTEX_POOL_SIZE] = {};
    struct SolidSyslogMutex* overflow                                    = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogWindowsMutex_Destroy(handle);
            }
        }
        // cppcheck-suppress knownConditionTrueFalse -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
        if (overflow != nullptr)
        {
            SolidSyslogWindowsMutex_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
        ErrorHandlerFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogWindowsMutex_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogWindowsMutexPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogWindowsMutex_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogWindowsMutexPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogWindowsMutex_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_WINDOWSMUTEX_POOL_EXHAUSTED, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogWindowsMutexPool, FallbackLockUnlockAreNoOps)
{
    FillPool();
    overflow = SolidSyslogWindowsMutex_Create();

    SolidSyslogMutex_Lock(overflow);
    SolidSyslogMutex_Unlock(overflow);
}

TEST(SolidSyslogWindowsMutexPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogWindowsMutex_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWindowsMutexPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogWindowsMutex_Create();

    LONGS_EQUAL(SOLIDSYSLOG_WINDOWS_MUTEX_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_WINDOWS_MUTEX_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogWindowsMutexPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogWindowsMutex_Create();
    ConfigLockFake_Install();

    SolidSyslogWindowsMutex_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWindowsMutexPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogMutex stranger = {};

    SolidSyslogWindowsMutex_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogWindowsMutexPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogMutex stranger = {};

    SolidSyslogWindowsMutex_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_WINDOWSMUTEX_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogWindowsMutexPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogWindowsMutex_Create();
    SolidSyslogWindowsMutex_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogWindowsMutex_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_WINDOWSMUTEX_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}
