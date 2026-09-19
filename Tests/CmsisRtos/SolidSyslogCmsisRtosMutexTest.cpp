#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "CmsisRtosMutexFake.h"
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogCmsisRtosMutex.h"
#include "SolidSyslogCmsisRtosMutexErrors.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogMutex.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

// Stands in for the RTOS mutex control block. Its size is arbitrary because no
// fake reads it — only a real implementation knows how big one has to be, which
// is why the caller supplies it.
struct TestControlBlock
{
    uint64_t words[8];
};

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
TEST_GROUP(SolidSyslogCmsisRtosMutex)
{
    struct SolidSyslogMutex* mutex = nullptr;
    TestControlBlock controlBlock  = {};

    void setup() override
    {
        CmsisRtosMutexFake_Reset();
        mutex = SolidSyslogCmsisRtosMutex_Create(&controlBlock, sizeof(controlBlock));
    }

    void teardown() override
    {
        SolidSyslogCmsisRtosMutex_Destroy(mutex);
    }
};

// clang-format on

TEST(SolidSyslogCmsisRtosMutex, CreateCallsMutexNewOnce)

{
    CALLED_FAKE(CmsisRtosMutexFake_MutexNew, ONCE);
}

TEST(SolidSyslogCmsisRtosMutex, CreateHandsTheCallersControlBlockToMutexNew)

{
    POINTERS_EQUAL(&controlBlock, CmsisRtosMutexFake_LastControlBlock());
}

TEST(SolidSyslogCmsisRtosMutex, CreateHandsTheCallersControlBlockSizeToMutexNew)

{
    UNSIGNED_LONGS_EQUAL(sizeof(controlBlock), CmsisRtosMutexFake_LastControlBlockBytes());
}

TEST(SolidSyslogCmsisRtosMutex, CreateAsksForPriorityInheritance)

{
    UNSIGNED_LONGS_EQUAL(osMutexPrioInherit, CmsisRtosMutexFake_LastAttrBits());
}

TEST(SolidSyslogCmsisRtosMutex, LockCallsMutexAcquireOnce)

{
    SolidSyslogMutex_Lock(mutex);

    CALLED_FAKE(CmsisRtosMutexFake_MutexAcquire, ONCE);
}

TEST(SolidSyslogCmsisRtosMutex, LockWaitsForever)

{
    SolidSyslogMutex_Lock(mutex);

    UNSIGNED_LONGS_EQUAL(osWaitForever, CmsisRtosMutexFake_LastAcquireTimeout());
}

TEST(SolidSyslogCmsisRtosMutex, UnlockCallsMutexReleaseOnce)

{
    SolidSyslogMutex_Unlock(mutex);

    CALLED_FAKE(CmsisRtosMutexFake_MutexRelease, ONCE);
}

TEST(SolidSyslogCmsisRtosMutex, DestroyCallsMutexDeleteOnce)

{
    SolidSyslogCmsisRtosMutex_Destroy(mutex);
    mutex = nullptr;

    CALLED_FAKE(CmsisRtosMutexFake_MutexDelete, ONCE);
}

// A guard rather than a driver. osMutexNew returns an opaque id that need not
// be the control block it was handed - CMSIS-FreeRTOS sets the low bit for a
// recursive mutex - so deriving the id from the control block would work on
// some implementations and corrupt others. The fake returns a deliberately
// unrelated pointer, which is what gives this test teeth.
TEST(SolidSyslogCmsisRtosMutex, DestroyDeletesTheIdMutexNewReturned)

{
    SolidSyslogCmsisRtosMutex_Destroy(mutex);
    mutex = nullptr;

    POINTERS_EQUAL(CmsisRtosMutexFake_LastCreatedId(), CmsisRtosMutexFake_LastDeletedId());
}

// An implementation refuses the control block it is handed when that block is
// smaller than the one it needs - a size only the integrator can know, since
// CMSIS-RTOS2 does not standardise it. osMutexNew answers NULL, and Create has
// to make that safe rather than hand back a mutex with no RTOS object behind it.
// clang-format off
TEST_GROUP(SolidSyslogCmsisRtosMutexRefused)
{
    struct SolidSyslogMutex* mutex = nullptr;
    TestControlBlock controlBlock  = {};

    void setup() override
    {
        CmsisRtosMutexFake_Reset();
        CmsisRtosMutexFake_SetMutexNewFails(true);
        mutex = SolidSyslogCmsisRtosMutex_Create(&controlBlock, sizeof(controlBlock));
    }

    void teardown() override
    {
        SolidSyslogCmsisRtosMutex_Destroy(mutex);
    }
};

// clang-format on

TEST(SolidSyslogCmsisRtosMutexRefused, LockAndUnlockAreNoOps)

{
    SolidSyslogMutex_Lock(mutex);
    SolidSyslogMutex_Unlock(mutex);

    CALLED_FAKE(CmsisRtosMutexFake_MutexAcquire, NEVER);
    CALLED_FAKE(CmsisRtosMutexFake_MutexRelease, NEVER);
}

TEST(SolidSyslogCmsisRtosMutexRefused, DestroyDeletesNothing)

{
    SolidSyslogCmsisRtosMutex_Destroy(mutex);
    mutex = nullptr;

    CALLED_FAKE(CmsisRtosMutexFake_MutexDelete, NEVER);
}

// clang-format off
TEST_GROUP(SolidSyslogCmsisRtosMutexPool)
{
    struct SolidSyslogMutex* pooled[SOLIDSYSLOG_MUTEX_POOL_SIZE] = {};
    struct SolidSyslogMutex* overflow                            = nullptr;
    TestControlBlock controlBlocks[SOLIDSYSLOG_MUTEX_POOL_SIZE + 1] = {};

    void setup() override
    {
        CmsisRtosMutexFake_Reset();
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogCmsisRtosMutex_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogCmsisRtosMutex_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    struct SolidSyslogMutex* CreateWithBlock(size_t which)
    {
        return SolidSyslogCmsisRtosMutex_Create(&controlBlocks[which], sizeof(TestControlBlock));
    }

    void FillPool()
    {
        for (size_t slot = 0; slot < SOLIDSYSLOG_MUTEX_POOL_SIZE; slot++)
        {
            pooled[slot] = CreateWithBlock(slot);
        }
    }
};

// clang-format on

TEST(SolidSyslogCmsisRtosMutexPool, FillingPoolThenOverflowReturnsDistinctFallback)

{
    FillPool();

    overflow = CreateWithBlock(SOLIDSYSLOG_MUTEX_POOL_SIZE);

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogCmsisRtosMutexPool, ExhaustedCreateReportsError)

{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = CreateWithBlock(SOLIDSYSLOG_MUTEX_POOL_SIZE);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogCmsisRtosMutexErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_MUTEX_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogCmsisRtosMutexPool, FallbackLockUnlockAreNoOps)

{
    FillPool();
    CmsisRtosMutexFake_Reset();
    overflow = CreateWithBlock(SOLIDSYSLOG_MUTEX_POOL_SIZE);

    SolidSyslogMutex_Lock(overflow);
    SolidSyslogMutex_Unlock(overflow);

    CALLED_FAKE(CmsisRtosMutexFake_MutexAcquire, NEVER);
    CALLED_FAKE(CmsisRtosMutexFake_MutexRelease, NEVER);
}

TEST(SolidSyslogCmsisRtosMutexPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)

{
    ConfigLockFake_Install();

    pooled[0] = CreateWithBlock(0);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogCmsisRtosMutexPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)

{
    FillPool();
    ConfigLockFake_Install();

    overflow = CreateWithBlock(SOLIDSYSLOG_MUTEX_POOL_SIZE);

    LONGS_EQUAL(SOLIDSYSLOG_MUTEX_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_MUTEX_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogCmsisRtosMutexPool, DestroyOfPooledHandleLocksOnce)

{
    pooled[0] = CreateWithBlock(0);
    ConfigLockFake_Install();

    SolidSyslogCmsisRtosMutex_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogCmsisRtosMutexPool, DestroyOfUnknownHandleDoesNotLock)

{
    ConfigLockFake_Install();
    struct SolidSyslogMutex stranger = {};

    SolidSyslogCmsisRtosMutex_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogCmsisRtosMutexPool, DestroyOfUnknownHandleReportsWarning)

{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogMutex stranger = {};

    SolidSyslogCmsisRtosMutex_Destroy(&stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogCmsisRtosMutexErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_MUTEX_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogCmsisRtosMutexPool, DestroyOfStaleHandleReportsWarning)

{
    pooled[0] = CreateWithBlock(0);
    SolidSyslogCmsisRtosMutex_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogCmsisRtosMutex_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogCmsisRtosMutexErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_MUTEX_ERROR_UNKNOWN_DESTROY
    );
}
