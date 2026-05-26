#include "ConfigLockFake.h"
#include "CppUTest/TestHarness.h"
#include "ErrorHandlerFake.h"
#include "SocketFake.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogPosixFile.h"
#include "SolidSyslogPosixFileErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "TestUtils.h"

#include <cstdio>

using namespace CososoTesting;

static const char* const TEST_PATH = "/tmp/test_posix_file.dat";

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
TEST_GROUP(SolidSyslogPosixFile)
{
    struct SolidSyslogFile* file = nullptr;

    void setup() override
    {
        SocketFake_Reset();
        remove(TEST_PATH);
        file = SolidSyslogPosixFile_Create();
    }

    void teardown() override
    {
        SolidSyslogPosixFile_Destroy(file);
        remove(TEST_PATH);
    }

    void OpenTestFile() const
    {
        CHECK_TRUE(SolidSyslogFile_Open(file, TEST_PATH));
    }
};

// clang-format on

TEST(SolidSyslogPosixFile, CreateReturnsNonNull)
{
    CHECK_TRUE(file != nullptr);
}

TEST(SolidSyslogPosixFile, IsOpenReturnsFalseBeforeOpen)
{
    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogPosixFile, OpenReturnsTrue)
{
    OpenTestFile();
}

TEST(SolidSyslogPosixFile, OpenSetsIsOpen)
{
    OpenTestFile();
    CHECK_TRUE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogPosixFile, CloseResetsIsOpen)
{
    OpenTestFile();
    SolidSyslogFile_Close(file);
    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogPosixFile, WriteAndReadRoundTrip)
{
    OpenTestFile();
    CHECK_TRUE(SolidSyslogFile_Write(file, "hello", 5));
    SolidSyslogFile_SeekTo(file, 0);

    char buf[16] = {};
    CHECK_TRUE(SolidSyslogFile_Read(file, buf, 5));
    MEMCMP_EQUAL("hello", buf, 5);
}

TEST(SolidSyslogPosixFile, SizeReturnsFileSize)
{
    OpenTestFile();
    LONGS_EQUAL(0, SolidSyslogFile_Size(file));
    CHECK_TRUE(SolidSyslogFile_Write(file, "hello", 5));
    LONGS_EQUAL(5, SolidSyslogFile_Size(file));
}

TEST(SolidSyslogPosixFile, TruncateClearsFile)
{
    OpenTestFile();
    CHECK_TRUE(SolidSyslogFile_Write(file, "hello", 5));
    SolidSyslogFile_Truncate(file);
    LONGS_EQUAL(0, SolidSyslogFile_Size(file));
}

TEST(SolidSyslogPosixFile, OpenWithInvalidPathReturnsFalse)
{
    CHECK_FALSE(SolidSyslogFile_Open(file, "/nonexistent/dir/file.dat"));
}

TEST(SolidSyslogPosixFile, DeleteRemovesFile)
{
    OpenTestFile();
    SolidSyslogFile_Close(file);
    CHECK_TRUE(SolidSyslogFile_Delete(file, TEST_PATH));
    CHECK_FALSE(SolidSyslogFile_Exists(file, TEST_PATH));
}

TEST(SolidSyslogPosixFile, DeleteReturnsFalseForNonexistentFile)
{
    CHECK_FALSE(SolidSyslogFile_Delete(file, "/tmp/nonexistent_posix_file.dat"));
}

// clang-format off
TEST_GROUP(SolidSyslogPosixFilePool)
{
    struct SolidSyslogFile* pooled[SOLIDSYSLOG_POSIX_FILE_POOL_SIZE] = {};
    struct SolidSyslogFile* overflow                                 = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogPosixFile_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogPosixFile_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogPosixFile_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogPosixFilePool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogPosixFile_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogPosixFilePool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogPosixFile_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PosixFileErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXFILE_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogPosixFilePool, FallbackOpenReturnsFalse)
{
    FillPool();
    overflow = SolidSyslogPosixFile_Create();

    CHECK_FALSE(SolidSyslogFile_Open(overflow, TEST_PATH));
}

TEST(SolidSyslogPosixFilePool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogPosixFile_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPosixFilePool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogPosixFile_Create();

    LONGS_EQUAL(SOLIDSYSLOG_POSIX_FILE_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_POSIX_FILE_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogPosixFilePool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogPosixFile_Create();
    ConfigLockFake_Install();

    SolidSyslogPosixFile_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPosixFilePool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogFile stranger = {};

    SolidSyslogPosixFile_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogPosixFilePool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogFile stranger = {};

    SolidSyslogPosixFile_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PosixFileErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXFILE_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogPosixFilePool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogPosixFile_Create();
    SolidSyslogPosixFile_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogPosixFile_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PosixFileErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXFILE_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
}
