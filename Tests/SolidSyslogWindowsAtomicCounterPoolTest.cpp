#include "CppUTest/TestHarness.h"

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogAtomicCounterDefinition.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWindowsAtomicCounter.h"
#include "SolidSyslogWindowsAtomicCounterErrors.h"
#include "TestUtils.h"

using namespace CososoTesting;

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
TEST_GROUP(SolidSyslogWindowsAtomicCounterPool)
{
    struct SolidSyslogAtomicCounter* pooled[SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE] = {};
    struct SolidSyslogAtomicCounter* overflow                                             = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogWindowsAtomicCounter_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogWindowsAtomicCounter_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogWindowsAtomicCounter_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogWindowsAtomicCounterPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogWindowsAtomicCounter_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogWindowsAtomicCounterPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogWindowsAtomicCounter_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&WindowsAtomicCounterErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_POOL_EXHAUSTED, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(WINDOWSATOMICCOUNTER_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWindowsAtomicCounterPool, FallbackIncrementReturnsOne)
{
    FillPool();
    overflow = SolidSyslogWindowsAtomicCounter_Create();

    LONGS_EQUAL(1U, SolidSyslogAtomicCounter_Increment(overflow));
}

TEST(SolidSyslogWindowsAtomicCounterPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogWindowsAtomicCounter_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWindowsAtomicCounterPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogWindowsAtomicCounter_Create();

    LONGS_EQUAL(SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogWindowsAtomicCounterPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogWindowsAtomicCounter_Create();
    ConfigLockFake_Install();

    SolidSyslogWindowsAtomicCounter_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWindowsAtomicCounterPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogAtomicCounter stranger = {};

    SolidSyslogWindowsAtomicCounter_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogWindowsAtomicCounterPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogAtomicCounter stranger = {};

    SolidSyslogWindowsAtomicCounter_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&WindowsAtomicCounterErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_UNKNOWN_DESTROY, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(WINDOWSATOMICCOUNTER_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWindowsAtomicCounterPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogWindowsAtomicCounter_Create();
    SolidSyslogWindowsAtomicCounter_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogWindowsAtomicCounter_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&WindowsAtomicCounterErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_UNKNOWN_DESTROY, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(WINDOWSATOMICCOUNTER_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastDetail());
}
