#include "CppUTest/TestHarness.h"
#include "TestUtils.h"

extern "C"
{
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "FatFsFake.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFatFsFile.h"
#include "SolidSyslogFatFsFileErrors.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
}

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
TEST_GROUP(SolidSyslogFatFsFilePool)
{
    struct SolidSyslogFile* pooled[SOLIDSYSLOG_FILE_POOL_SIZE] = {};
    struct SolidSyslogFile* overflow                                 = nullptr;

    void setup() override
    {
        FatFsFake_Reset();
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogFatFsFile_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogFatFsFile_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogFatFsFile_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogFatFsFilePool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogFatFsFile_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogFatFsFilePool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogFatFsFile_Create();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &FatFsFileErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_FATFS_FILE_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogFatFsFilePool, FallbackOpenReturnsFalse)
{
    FillPool();
    overflow = SolidSyslogFatFsFile_Create();

    CHECK_FALSE(SolidSyslogFile_Open(overflow, "anything.log"));
}

TEST(SolidSyslogFatFsFilePool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogFatFsFile_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogFatFsFilePool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogFatFsFile_Create();

    LONGS_EQUAL(SOLIDSYSLOG_FILE_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_FILE_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogFatFsFilePool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogFatFsFile_Create();
    ConfigLockFake_Install();

    SolidSyslogFatFsFile_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogFatFsFilePool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogFile stranger = {};

    SolidSyslogFatFsFile_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogFatFsFilePool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogFile stranger = {};

    SolidSyslogFatFsFile_Destroy(&stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &FatFsFileErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_FATFS_FILE_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogFatFsFilePool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogFatFsFile_Create();
    SolidSyslogFatFsFile_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogFatFsFile_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &FatFsFileErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_FATFS_FILE_ERROR_UNKNOWN_DESTROY
    );
}
