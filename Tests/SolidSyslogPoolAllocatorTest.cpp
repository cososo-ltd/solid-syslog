#include <stddef.h>

#include "CppUTest/TestHarness.h"

#include "ConfigLockFake.h"
#include "SolidSyslogPoolAllocator.h"
#include "TestUtils.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings ONCE/NEVER into scope for CALLED_FAKE

namespace
{
constexpr size_t TEST_POOL_SIZE = 3;
}

static int cleanupSpyCallCount;
static size_t cleanupSpyLastIndex;
static int cleanupSpyLockDeltaAtCall;

static void CleanupSpy(size_t index, void* context)
{
    (void) context;
    cleanupSpyCallCount++;
    cleanupSpyLastIndex = index;
    cleanupSpyLockDeltaAtCall = ConfigLockFake_LockCallCount() - ConfigLockFake_UnlockCallCount();
}

// clang-format off
TEST_GROUP(SolidSyslogPoolAllocator)
{
    bool                            inUse[TEST_POOL_SIZE] = {};
    struct SolidSyslogPoolAllocator allocator             = {inUse, TEST_POOL_SIZE};

    void setup() override
    {
        cleanupSpyCallCount       = 0;
        cleanupSpyLastIndex       = 0;
        cleanupSpyLockDeltaAtCall = 0;
    }

    void teardown() override
    {
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto& flag : inUse)
        {
            flag = true;
        }
    }
};

// clang-format on

TEST(SolidSyslogPoolAllocator, IndexAtCountIsInvalid)
{
    CHECK_FALSE(SolidSyslogPoolAllocator_IndexIsValid(&allocator, TEST_POOL_SIZE));
}

TEST(SolidSyslogPoolAllocator, IndexBelowCountIsValid)
{
    CHECK_TRUE(SolidSyslogPoolAllocator_IndexIsValid(&allocator, TEST_POOL_SIZE - 1));
}

TEST(SolidSyslogPoolAllocator, AcquireFirstFreeOnEmptyPoolReturnsZeroAndMarksItInUse)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&allocator);

    LONGS_EQUAL(0, index);
    CHECK_TRUE(inUse[0]);
}

TEST(SolidSyslogPoolAllocator, AcquireFirstFreeWalksPastInUseSlots)
{
    inUse[0] = true;

    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&allocator);

    LONGS_EQUAL(1, index);
    CHECK_TRUE(inUse[1]);
}

TEST(SolidSyslogPoolAllocator, AcquireFirstFreeOnExhaustedPoolReturnsCount)
{
    FillPool();

    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&allocator);

    LONGS_EQUAL(TEST_POOL_SIZE, index);
}

TEST(SolidSyslogPoolAllocator, FreeIfInUseOnInUseSlotReleasesItAndInvokesCleanup)
{
    inUse[1] = true;

    bool released = SolidSyslogPoolAllocator_FreeIfInUse(&allocator, 1, CleanupSpy, nullptr);

    CHECK_TRUE(released);
    CHECK_FALSE(inUse[1]);
    CALLED_FUNCTION(cleanupSpy, ONCE);
    LONGS_EQUAL(1, cleanupSpyLastIndex);
}

TEST(SolidSyslogPoolAllocator, FreeIfInUseOnAlreadyFreeSlotReturnsFalseAndSkipsCleanup)
{
    bool released = SolidSyslogPoolAllocator_FreeIfInUse(&allocator, 0, CleanupSpy, nullptr);

    CHECK_FALSE(released);
    CALLED_FUNCTION(cleanupSpy, NEVER);
}

TEST(SolidSyslogPoolAllocator, FreeIfInUseWithNullCleanupStillReleasesSlot)
{
    inUse[2] = true;

    bool released = SolidSyslogPoolAllocator_FreeIfInUse(&allocator, 2, nullptr, nullptr);

    CHECK_TRUE(released);
    CHECK_FALSE(inUse[2]);
}

TEST(SolidSyslogPoolAllocator, AcquireFirstFreeLocksOnceForOneProbedSlot)
{
    ConfigLockFake_Install();

    SolidSyslogPoolAllocator_AcquireFirstFree(&allocator);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPoolAllocator, AcquireFirstFreeLocksOncePerSlotWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    SolidSyslogPoolAllocator_AcquireFirstFree(&allocator);

    LONGS_EQUAL(TEST_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(TEST_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogPoolAllocator, FreeIfInUseLocksOnce)
{
    inUse[1] = true;
    ConfigLockFake_Install();

    SolidSyslogPoolAllocator_FreeIfInUse(&allocator, 1, CleanupSpy, nullptr);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPoolAllocator, FreeIfInUseInvokesCleanupWhileHoldingTheLock)
{
    inUse[1] = true;
    ConfigLockFake_Install();

    SolidSyslogPoolAllocator_FreeIfInUse(&allocator, 1, CleanupSpy, nullptr);

    LONGS_EQUAL(1, cleanupSpyLockDeltaAtCall);
}
