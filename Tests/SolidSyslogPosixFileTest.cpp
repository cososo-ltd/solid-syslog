#include "ConfigLockFake.h"
#include "CppUTest/TestHarness.h"
#include "ErrorHandlerFake.h"
#include "SocketFake.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogFileCategories.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogPosixFile.h"
#include "SolidSyslogPosixFileErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogFileErrors.h"
#include "SolidSyslogError.h"
#include "TestUtils.h"

#include <cerrno>
#include <cstdio>
#include <sys/resource.h>
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

// Asserts the call raised exactly the File fault naming this operation, with
// the errno behind it.
#define CHECK_FAILURE_REPORTED_WITH_ERRNO(code, nativeErrno) \
    CHECK_ERROR_REPORTED_ONCE_WITH_NATIVE_CODE(              \
        SOLIDSYSLOG_SEVERITY_ERROR,                          \
        &SolidSyslogPosixFileErrorSource,                    \
        SOLIDSYSLOG_CAT_FILE_IO_FAILED,                      \
        code,                                                \
        nativeErrno                                          \
    )

// Asserts the call raised exactly the File fault naming this operation, with
// no errno behind it.
#define CHECK_FAILURE_REPORTED_ALONE(code) \
    CHECK_ERROR_REPORTED_ONCE(             \
        SOLIDSYSLOG_SEVERITY_ERROR,        \
        &SolidSyslogPosixFileErrorSource,  \
        SOLIDSYSLOG_CAT_FILE_IO_FAILED,    \
        code                               \
    )

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

TEST(SolidSyslogPosixFile, DeleteReturnsTrueForNonexistentFile)
{
    CHECK_TRUE(SolidSyslogFile_Delete(file, "/tmp/nonexistent_posix_file.dat"));
}

TEST(SolidSyslogPosixFile, DeleteReturnsFalseForAPathItCannotRemove)
{
    CHECK_FALSE(SolidSyslogFile_Delete(file, "/tmp"));
}

TEST(SolidSyslogPosixFile, OpenThatFailsReportsTheFailedOpenAndItsErrno)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogFile_Open(file, "/nonexistent/dir/file.dat");

    CHECK_FAILURE_REPORTED_WITH_ERRNO(SOLIDSYSLOG_FILE_ERROR_OPEN_FAILED, ENOENT);
}

// Every call on a file that is not open fails with EBADF, which is how each
// failing call is driven: the real file system refuses nothing else on demand.
TEST(SolidSyslogPosixFile, ReadThatFailsReportsTheFailedReadAndItsErrno)
{
    char buf[5] = {};
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogFile_Read(file, buf, sizeof(buf));

    CHECK_FAILURE_REPORTED_WITH_ERRNO(SOLIDSYSLOG_FILE_ERROR_READ_FAILED, EBADF);
}

TEST(SolidSyslogPosixFile, AShortReadReportsNothing)
{
    char buf[16] = {};
    OpenTestFile();
    CHECK_TRUE(SolidSyslogFile_Write(file, "hello", 5));
    SolidSyslogFile_SeekTo(file, 0);
    ErrorHandlerFake_Install(nullptr);

    CHECK_FALSE(SolidSyslogFile_Read(file, buf, sizeof(buf)));

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogPosixFile, WriteThatFailsReportsTheFailedWriteAndItsErrno)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogFile_Write(file, "hello", 5);

    CHECK_FAILURE_REPORTED_WITH_ERRNO(SOLIDSYSLOG_FILE_ERROR_WRITE_FAILED, EBADF);
}

// A regular file takes a write short, with no errno, only when the process's
// file-size limit falls inside it. The limit is the soft one, lowered for the
// write and restored straight after; nothing else in the process writes a
// file in between, and a write that fits under the limit entirely is not
// affected.
TEST(SolidSyslogPosixFile, AShortWriteReportsTheFailedWriteAlone)
{
    struct rlimit previous = {};
    LONGS_EQUAL(0, getrlimit(RLIMIT_FSIZE, &previous));
    struct rlimit threeBytes = {3, previous.rlim_max};
    OpenTestFile();
    ErrorHandlerFake_Install(nullptr);
    LONGS_EQUAL(0, setrlimit(RLIMIT_FSIZE, &threeBytes));

    bool wrote = SolidSyslogFile_Write(file, "hello", 5);

    LONGS_EQUAL(0, setrlimit(RLIMIT_FSIZE, &previous));
    CHECK_FALSE(wrote);
    CHECK_FAILURE_REPORTED_ALONE(SOLIDSYSLOG_FILE_ERROR_WRITE_FAILED);
}

TEST(SolidSyslogPosixFile, SeekToThatFailsReportsTheFailedSeekAndItsErrno)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogFile_SeekTo(file, 42);

    CHECK_FAILURE_REPORTED_WITH_ERRNO(SOLIDSYSLOG_FILE_ERROR_SEEK_FAILED, EBADF);
}

TEST(SolidSyslogPosixFile, SizeThatFailsReportsTheFailedSizeAndItsErrno)
{
    LONGS_EQUAL(0, mkfifo(TEST_FIFO_PATH, S_IRUSR | S_IWUSR));
    CHECK_TRUE(SolidSyslogFile_Open(file, TEST_FIFO_PATH));
    ErrorHandlerFake_Install(nullptr);

    size_t size = 0U;
    SolidSyslogFile_Size(file, &size);

    CHECK_FAILURE_REPORTED_WITH_ERRNO(SOLIDSYSLOG_FILE_ERROR_SIZE_FAILED, ESPIPE);
}

TEST(SolidSyslogPosixFile, TruncateThatFailsReportsTheFailedTruncateAndItsErrno)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogFile_Truncate(file);

    CHECK_FAILURE_REPORTED_WITH_ERRNO(SOLIDSYSLOG_FILE_ERROR_TRUNCATE_FAILED, EBADF);
}

TEST(SolidSyslogPosixFile, CloseThatFailsReportsTheFailedCloseAndItsErrno)
{
    OpenTestFile();
    ErrorHandlerFake_Install(nullptr);
    SocketFake_FailNextCloseWithErrno(EIO);

    SolidSyslogFile_Close(file);

    CHECK_FAILURE_REPORTED_WITH_ERRNO(SOLIDSYSLOG_FILE_ERROR_CLOSE_FAILED, EIO);
}

TEST(SolidSyslogPosixFile, CloseThatFailsStillLeavesTheFileClosed)
{
    OpenTestFile();
    SocketFake_FailNextCloseWithErrno(EIO);

    SolidSyslogFile_Close(file);

    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogPosixFile, CallsThatSucceedReportNothing)
{
    char buf[5] = {};
    ErrorHandlerFake_Install(nullptr);
    OpenTestFile();

    size_t size = 0U;
    SolidSyslogFile_Write(file, "hello", 5);
    SolidSyslogFile_Size(file, &size);
    SolidSyslogFile_SeekTo(file, 0);
    SolidSyslogFile_Read(file, buf, sizeof(buf));
    SolidSyslogFile_Truncate(file);
    SolidSyslogFile_Close(file);

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
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

TEST(SolidSyslogPosixFile, TheErrorSourceNamesItsDetailsThroughFileErrors)
{
    CHECK(SolidSyslogFileErrors_Name == SolidSyslogPosixFileErrorSource.DetailName);
}
