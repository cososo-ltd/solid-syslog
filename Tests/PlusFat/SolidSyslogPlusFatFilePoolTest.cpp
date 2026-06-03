#include "CppUTest/TestHarness.h"
#include "TestUtils.h"

extern "C"
{
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "PlusFatFake.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogPlusFatFile.h"
#include "SolidSyslogPlusFatFileErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
}

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
TEST_GROUP(SolidSyslogPlusFatFilePool)
{
    struct SolidSyslogFile* pooled[SOLIDSYSLOG_FILE_POOL_SIZE] = {};
    struct SolidSyslogFile* overflow                                 = nullptr;

    void setup() override
    {
        PlusFatFake_Reset();
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogPlusFatFile_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogPlusFatFile_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogPlusFatFile_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogPlusFatFilePool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogPlusFatFile_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogPlusFatFilePool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogPlusFatFile_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PlusFatFileErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_POOL_EXHAUSTED, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(PLUSFATFILE_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogPlusFatFilePool, FallbackOpenReturnsFalse)
{
    FillPool();
    overflow = SolidSyslogPlusFatFile_Create();

    CHECK_FALSE(SolidSyslogFile_Open(overflow, "anything.log"));
}

TEST(SolidSyslogPlusFatFilePool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogPlusFatFile_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPlusFatFilePool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogPlusFatFile_Create();

    LONGS_EQUAL(SOLIDSYSLOG_FILE_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_FILE_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogPlusFatFilePool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogPlusFatFile_Create();
    ConfigLockFake_Install();

    SolidSyslogPlusFatFile_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPlusFatFilePool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogFile stranger = {};

    SolidSyslogPlusFatFile_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogPlusFatFilePool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogFile stranger = {};

    SolidSyslogPlusFatFile_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PlusFatFileErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_UNKNOWN_DESTROY, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(PLUSFATFILE_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogPlusFatFilePool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogPlusFatFile_Create();
    SolidSyslogPlusFatFile_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogPlusFatFile_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PlusFatFileErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_UNKNOWN_DESTROY, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(PLUSFATFILE_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastDetail());
}
