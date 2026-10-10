#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogAtomicCounterDefinition.h"
#include "SolidSyslogAtomicCounterErrors.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogVxWorks64AtomicCounter.h"
#include "SolidSyslogVxWorks64AtomicCounterErrors.h"
#include "VxWorks64IntFake.h"

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
TEST_GROUP(SolidSyslogVxWorks64AtomicCounterPool)
{
    struct SolidSyslogAtomicCounter* pooled[SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE] = {};
    struct SolidSyslogAtomicCounter* overflow                                     = nullptr;

    void setup() override
    {
        VxWorks64IntFake_Reset();
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogVxWorks64AtomicCounter_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogVxWorks64AtomicCounter_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogVxWorks64AtomicCounter_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64AtomicCounterPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogVxWorks64AtomicCounter_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogVxWorks64AtomicCounterPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogVxWorks64AtomicCounter_Create();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogVxWorks64AtomicCounterErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_ATOMIC_COUNTER_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogVxWorks64AtomicCounterPool, FallbackIncrementReturnsOne)
{
    FillPool();
    overflow = SolidSyslogVxWorks64AtomicCounter_Create();

    LONGS_EQUAL(1U, SolidSyslogAtomicCounter_Increment(overflow));
}

TEST(SolidSyslogVxWorks64AtomicCounterPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogVxWorks64AtomicCounter_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogVxWorks64AtomicCounterPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogVxWorks64AtomicCounter_Create();

    LONGS_EQUAL(SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogVxWorks64AtomicCounterPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogVxWorks64AtomicCounter_Create();
    ConfigLockFake_Install();

    SolidSyslogVxWorks64AtomicCounter_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogVxWorks64AtomicCounterPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogAtomicCounter stranger = {};

    SolidSyslogVxWorks64AtomicCounter_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogVxWorks64AtomicCounterPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogAtomicCounter stranger = {};

    SolidSyslogVxWorks64AtomicCounter_Destroy(&stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogVxWorks64AtomicCounterErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_ATOMIC_COUNTER_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogVxWorks64AtomicCounterPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogVxWorks64AtomicCounter_Create();
    SolidSyslogVxWorks64AtomicCounter_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogVxWorks64AtomicCounter_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogVxWorks64AtomicCounterErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_ATOMIC_COUNTER_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogVxWorks64AtomicCounterPool, IncrementAfterDestroyActsAsNullCounter)
{
    struct SolidSyslogAtomicCounter* stale = SolidSyslogVxWorks64AtomicCounter_Create();
    (void) SolidSyslogAtomicCounter_Increment(stale);
    (void) SolidSyslogAtomicCounter_Increment(stale);
    SolidSyslogVxWorks64AtomicCounter_Destroy(stale);

    LONGS_EQUAL(1U, SolidSyslogAtomicCounter_Increment(stale));
}
