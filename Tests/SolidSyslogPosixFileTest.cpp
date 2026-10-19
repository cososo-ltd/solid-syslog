#include "ConfigLockFake.h"
#include "CppUTest/TestHarness.h"
#include "ErrorHandlerFake.h"
#include "SocketFake.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogPosixFile.h"
#include "SolidSyslogPosixFileErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "TestUtils.h"

#include <cstdio>
#include <sys/stat.h>

using namespace CososoTesting;

static const char* const TEST_PATH = "/tmp/test_posix_file.dat";
/* A FIFO: open succeeds, and lseek on it fails with ESPIPE. */
static const char* const TEST_FIFO_PATH = "/tmp/test_posix_file.fifo";

#define CHECK_IS_FALLBACK(handle, pool)                                                \
    {                                                                                  \
        CHECK_TEXT((handle) != nullptr, "Fallback handle was nullptr");                \
        for (auto* slot : (pool))                                                      \
        {                                                                              \
            CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");   \
            CHECK_TEXT((handle) != slot, "Fallback handle collided with a pool slot"); \
        }                                                                              \
    }

/* Asserts the open file reports `expected` bytes. A macro so a failure
 * reports the caller's __FILE__/__LINE__. */
#define CHECK_FILE_SIZE(file, expected)                       \
    {                                                         \
        size_t checkSize = 0U;                                \
        CHECK_TRUE(SolidSyslogFile_Size((file), &checkSize)); \
        LONGS_EQUAL((expected), checkSize);                   \
    }

// clang-format off
TEST_GROUP(SolidSyslogPosixFile)
{
    struct SolidSyslogFile* file = nullptr;

    void setup() override
    {
        SocketFake_Reset();
        remove(TEST_PATH);
        remove(TEST_FIFO_PATH);
        file = SolidSyslogPosixFile_Create();
    }

    void teardown() override
    {
        SolidSyslogPosixFile_Destroy(file);
        remove(TEST_PATH);
        remove(TEST_FIFO_PATH);
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
    CHECK_FILE_SIZE(file, 0);
    CHECK_TRUE(SolidSyslogFile_Write(file, "hello", 5));
    CHECK_FILE_SIZE(file, 5);
}

TEST(SolidSyslogPosixFile, SizeFailsOnAFileThatCannotSeek)
{
    LONGS_EQUAL(0, mkfifo(TEST_FIFO_PATH, S_IRUSR | S_IWUSR));
    CHECK_TRUE(SolidSyslogFile_Open(file, TEST_FIFO_PATH));

    size_t size = 42U;
    CHECK_FALSE(SolidSyslogFile_Size(file, &size));
    LONGS_EQUAL(0, size);
}

TEST(SolidSyslogPosixFile, TruncateClearsFile)
{
    OpenTestFile();
    CHECK_TRUE(SolidSyslogFile_Write(file, "hello", 5));
    SolidSyslogFile_Truncate(file);
    CHECK_FILE_SIZE(file, 0);
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
    struct SolidSyslogFile* pooled[SOLIDSYSLOG_FILE_POOL_SIZE] = {};
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

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogPosixFileErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_FILE_ERROR_POOL_EXHAUSTED
    );
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

    LONGS_EQUAL(SOLIDSYSLOG_FILE_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_FILE_POOL_SIZE, ConfigLockFake_UnlockCallCount());
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

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogPosixFileErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_FILE_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogPosixFilePool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogPosixFile_Create();
    SolidSyslogPosixFile_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogPosixFile_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogPosixFileErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_FILE_ERROR_UNKNOWN_DESTROY
    );
}
