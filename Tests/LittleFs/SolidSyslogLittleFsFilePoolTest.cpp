#include "CppUTest/TestHarness.h"
#include "TestUtils.h"

extern "C"
{
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "LittleFsFake.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogFileErrors.h"
#include "SolidSyslogLittleFsFile.h"
#include "SolidSyslogLittleFsFileErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "lfs.h"
}

using namespace CososoTesting;

static const lfs_size_t POOL_TEST_CACHE_SIZE = 32;

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
TEST_GROUP(SolidSyslogLittleFsFilePool)
{
    struct SolidSyslogFile* pooled[SOLIDSYSLOG_FILE_POOL_SIZE] = {};
    struct SolidSyslogFile* overflow = nullptr;
    lfs_t* filesystem = nullptr;
    // One buffer per slot, plus one the overflow Create is offered.
    unsigned char buffers[SOLIDSYSLOG_FILE_POOL_SIZE + 1][POOL_TEST_CACHE_SIZE] = {};

    void setup() override
    {
        LittleFsFake_Reset();
        filesystem = LittleFsFake_MountedWithCacheSize(POOL_TEST_CACHE_SIZE);
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogLittleFsFile_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogLittleFsFile_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    struct SolidSyslogFile* CreateWithBuffer(unsigned char* buffer) const
    {
        return SolidSyslogLittleFsFile_Create(filesystem, buffer, POOL_TEST_CACHE_SIZE);
    }

    // Walks buffers alongside pooled rather than indexing either: a subscript
    // whose index is not a constant expression is a clang-tidy error here.
    void FillPool()
    {
        auto* buffer = buffers;
        for (auto*& slot : pooled)
        {
            slot = CreateWithBuffer(*buffer);
            ++buffer;
        }
    }
};

// clang-format on

TEST(SolidSyslogLittleFsFilePool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = CreateWithBuffer(buffers[SOLIDSYSLOG_FILE_POOL_SIZE]);

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogLittleFsFilePool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = CreateWithBuffer(buffers[SOLIDSYSLOG_FILE_POOL_SIZE]);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogLittleFsFileErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_FILE_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogLittleFsFilePool, FallbackOpenReturnsFalse)
{
    FillPool();
    overflow = CreateWithBuffer(buffers[SOLIDSYSLOG_FILE_POOL_SIZE]);

    CHECK_FALSE(SolidSyslogFile_Open(overflow, "anything.log"));
}

TEST(SolidSyslogLittleFsFilePool, ARefusedCreateLeavesItsSlotFree)
{
    struct SolidSyslogFile* refused = SolidSyslogLittleFsFile_Create(nullptr, buffers[0], POOL_TEST_CACHE_SIZE);
    (void) refused;

    FillPool();

    for (auto* slot : pooled)
    {
        CHECK_TEXT(SolidSyslogFile_Open(slot, "after-refusal.log"), "a refused Create had kept its slot");
    }
}

TEST(SolidSyslogLittleFsFilePool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = CreateWithBuffer(buffers[0]);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogLittleFsFilePool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = CreateWithBuffer(buffers[SOLIDSYSLOG_FILE_POOL_SIZE]);

    LONGS_EQUAL(SOLIDSYSLOG_FILE_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_FILE_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}
