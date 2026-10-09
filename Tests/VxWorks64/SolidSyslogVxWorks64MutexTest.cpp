#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "vxWorks.h"

#include "semLib.h"

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogMutex.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogMutexErrors.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogVxWorks64Mutex.h"
#include "SolidSyslogVxWorks64MutexErrors.h"
#include "VxWorks64SemFake.h"

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
TEST_GROUP(SolidSyslogVxWorks64Mutex)
{
    struct SolidSyslogMutex* mutex = nullptr;

    void setup() override
    {
        VxWorks64SemFake_Reset();
        mutex = SolidSyslogVxWorks64Mutex_Create();
    }

    void teardown() override
    {
        SolidSyslogVxWorks64Mutex_Destroy(mutex);
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64Mutex, CreateCallsSemMCreateOnce)
{
    CALLED_FAKE(VxWorks64SemFake_SemMCreate, ONCE);
}

TEST(SolidSyslogVxWorks64Mutex, CreateAsksForPriorityInheritanceAndDeletionSafety)
{
    LONGS_EQUAL(SEM_Q_PRIORITY | SEM_INVERSION_SAFE | SEM_DELETE_SAFE, VxWorks64SemFake_LastSemMCreateOptions());
}

TEST(SolidSyslogVxWorks64Mutex, CreateReturnsAMutexThatIsNotTheNullMutex)
{
    CHECK(mutex != SolidSyslogNullMutex_Get());
}

TEST(SolidSyslogVxWorks64Mutex, LockTakesTheCreatedSemaphore)
{
    SolidSyslogMutex_Lock(mutex);
    CALLED_FAKE(VxWorks64SemFake_SemTake, ONCE);
    POINTERS_EQUAL(VxWorks64SemFake_LastCreatedId(), VxWorks64SemFake_LastTakenId());
}

TEST(SolidSyslogVxWorks64Mutex, LockWaitsForever)
{
    SolidSyslogMutex_Lock(mutex);
    LONGS_EQUAL(WAIT_FOREVER, VxWorks64SemFake_LastTakeTimeout());
}

TEST(SolidSyslogVxWorks64Mutex, UnlockGivesTheCreatedSemaphore)
{
    SolidSyslogMutex_Unlock(mutex);
    CALLED_FAKE(VxWorks64SemFake_SemGive, ONCE);
    POINTERS_EQUAL(VxWorks64SemFake_LastCreatedId(), VxWorks64SemFake_LastGivenId());
}

TEST(SolidSyslogVxWorks64Mutex, DestroyDeletesTheCreatedSemaphore)
{
    SolidSyslogVxWorks64Mutex_Destroy(mutex);
    mutex = nullptr;

    CALLED_FAKE(VxWorks64SemFake_SemDelete, ONCE);
    POINTERS_EQUAL(VxWorks64SemFake_LastCreatedId(), VxWorks64SemFake_LastDeletedId());
}

TEST(SolidSyslogVxWorks64Mutex, LockAfterDestroyLeavesTheSemaphoreAlone)
{
    struct SolidSyslogMutex* stale = mutex;
    SolidSyslogVxWorks64Mutex_Destroy(mutex);
    mutex = nullptr;

    SolidSyslogMutex_Lock(stale);
    SolidSyslogMutex_Unlock(stale);

    CALLED_FAKE(VxWorks64SemFake_SemTake, NEVER);
    CALLED_FAKE(VxWorks64SemFake_SemGive, NEVER);
}

// semMCreate answers NULL when the kernel cannot allocate the semaphore, and
// Create has to make that safe rather than hand back a mutex with no semaphore
// behind it.
// clang-format off
TEST_GROUP(SolidSyslogVxWorks64MutexRefused)
{
    struct SolidSyslogMutex* mutex = nullptr;

    void setup() override
    {
        VxWorks64SemFake_Reset();
        VxWorks64SemFake_SetSemMCreateFails(true);
        mutex = SolidSyslogVxWorks64Mutex_Create();
    }

    // No teardown: a refused create holds no slot.
};

// clang-format on

TEST(SolidSyslogVxWorks64MutexRefused, CreateReturnsTheSharedNullMutex)
{
    POINTERS_EQUAL(SolidSyslogNullMutex_Get(), mutex);
}

TEST(SolidSyslogVxWorks64MutexRefused, CreateDeletesNothingOnItsWayOut)
{
    CALLED_FAKE(VxWorks64SemFake_SemDelete, NEVER);
}

TEST(SolidSyslogVxWorks64MutexRefused, LockAndUnlockAreNoOps)
{
    SolidSyslogMutex_Lock(mutex);
    SolidSyslogMutex_Unlock(mutex);

    CALLED_FAKE(VxWorks64SemFake_SemTake, NEVER);
    CALLED_FAKE(VxWorks64SemFake_SemGive, NEVER);
}

// A kernel that cannot allocate one semaphore will refuse the next too; a held
// slot would let a retrying caller empty the pool.
TEST(SolidSyslogVxWorks64MutexRefused, LeavesThePoolAvailable)
{
    VxWorks64SemFake_SetSemMCreateFails(false);

    struct SolidSyslogMutex* second = SolidSyslogVxWorks64Mutex_Create();
    SolidSyslogMutex_Lock(second);
    SolidSyslogVxWorks64Mutex_Destroy(second);

    CALLED_FAKE(VxWorks64SemFake_SemTake, ONCE);
}

TEST(SolidSyslogVxWorks64MutexRefused, CreateReportsCritical)
{
    ErrorHandlerFake_Install(nullptr);

    mutex = SolidSyslogVxWorks64Mutex_Create();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogVxWorks64MutexErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_MUTEX_ERROR_CREATE_FAILED
    );
}

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64MutexPool)
{
    struct SolidSyslogMutex* pooled[SOLIDSYSLOG_MUTEX_POOL_SIZE] = {};
    struct SolidSyslogMutex* overflow                            = nullptr;

    void setup() override
    {
        VxWorks64SemFake_Reset();
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogVxWorks64Mutex_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogVxWorks64Mutex_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogVxWorks64Mutex_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64MutexPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogVxWorks64Mutex_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogVxWorks64MutexPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogVxWorks64Mutex_Create();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogVxWorks64MutexErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_MUTEX_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogVxWorks64MutexPool, FallbackLockUnlockAreNoOps)
{
    FillPool();
    VxWorks64SemFake_Reset();
    overflow = SolidSyslogVxWorks64Mutex_Create();

    SolidSyslogMutex_Lock(overflow);
    SolidSyslogMutex_Unlock(overflow);

    CALLED_FAKE(VxWorks64SemFake_SemTake, NEVER);
    CALLED_FAKE(VxWorks64SemFake_SemGive, NEVER);
}

TEST(SolidSyslogVxWorks64MutexPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogVxWorks64Mutex_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogVxWorks64MutexPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogVxWorks64Mutex_Create();

    LONGS_EQUAL(SOLIDSYSLOG_MUTEX_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_MUTEX_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogVxWorks64MutexPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogVxWorks64Mutex_Create();
    ConfigLockFake_Install();

    SolidSyslogVxWorks64Mutex_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogVxWorks64MutexPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogMutex stranger = {};

    SolidSyslogVxWorks64Mutex_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogVxWorks64MutexPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogMutex stranger = {};

    SolidSyslogVxWorks64Mutex_Destroy(&stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogVxWorks64MutexErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_MUTEX_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogVxWorks64MutexPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogVxWorks64Mutex_Create();
    SolidSyslogVxWorks64Mutex_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogVxWorks64Mutex_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogVxWorks64MutexErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_MUTEX_ERROR_UNKNOWN_DESTROY
    );
}
