#include "CppUTest/TestHarness.h"

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogAtomicCounterDefinition.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStdAtomicCounter.h"
#include "SolidSyslogStdAtomicCounterErrors.h"
#include "SolidSyslogTunables.h"
#include "TestUtils.h"

using namespace CososoTesting;

// Asserts handle is non-null and not one of the slots in pool.
#define CHECK_IS_FALLBACK(handle, pool)                                                \
    {                                                                                  \
        CHECK_TEXT((handle) != nullptr, "Fallback handle was nullptr");                \
        for (auto* slot : (pool))                                                      \
        {                                                                              \
            CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");   \
            CHECK_TEXT((handle) != slot, "Fallback handle collided with a pool slot"); \
        }                                                                              \
    }

// clang-format off
TEST_GROUP(SolidSyslogStdAtomicCounterPool)
{
    struct SolidSyslogAtomicCounter* pooled[SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE] = {};
    struct SolidSyslogAtomicCounter* overflow                                         = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogStdAtomicCounter_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogStdAtomicCounter_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogStdAtomicCounter_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogStdAtomicCounterPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogStdAtomicCounter_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogStdAtomicCounterPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogStdAtomicCounter_Create();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &StdAtomicCounterErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_STDATOMIC_COUNTER_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogStdAtomicCounterPool, FallbackIncrementReturnsOne)
{
    FillPool();
    overflow = SolidSyslogStdAtomicCounter_Create();

    LONGS_EQUAL(1U, SolidSyslogAtomicCounter_Increment(overflow));
}

TEST(SolidSyslogStdAtomicCounterPool, ReacquiredSlotRestartsCountingFromOne)
{
    pooled[0] = SolidSyslogStdAtomicCounter_Create();
    SolidSyslogAtomicCounter_Increment(pooled[0]);
    SolidSyslogAtomicCounter_Increment(pooled[0]);
    SolidSyslogStdAtomicCounter_Destroy(pooled[0]);

    pooled[0] = SolidSyslogStdAtomicCounter_Create();

    LONGS_EQUAL(1U, SolidSyslogAtomicCounter_Increment(pooled[0]));
}

TEST(SolidSyslogStdAtomicCounterPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogStdAtomicCounter_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogStdAtomicCounterPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogStdAtomicCounter_Create();

    LONGS_EQUAL(SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogStdAtomicCounterPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogStdAtomicCounter_Create();
    ConfigLockFake_Install();

    SolidSyslogStdAtomicCounter_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogStdAtomicCounterPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogAtomicCounter stranger = {};

    SolidSyslogStdAtomicCounter_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogStdAtomicCounterPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogAtomicCounter stranger = {};

    SolidSyslogStdAtomicCounter_Destroy(&stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &StdAtomicCounterErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_STDATOMIC_COUNTER_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogStdAtomicCounterPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogStdAtomicCounter_Create();
    SolidSyslogStdAtomicCounter_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogStdAtomicCounter_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &StdAtomicCounterErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_STDATOMIC_COUNTER_ERROR_UNKNOWN_DESTROY
    );
}
