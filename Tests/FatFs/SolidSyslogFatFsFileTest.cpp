#include "CppUTest/TestHarness.h"
#include "TestUtils.h"

extern "C"
{
#include "ErrorHandlerFake.h"
#include "FatFsFake.h"
#include "SolidSyslogFatFsFile.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogFileCategories.h"
#include "SolidSyslogFileErrors.h"
#include "SolidSyslogError.h"
#include "SolidSyslogFatFsFileErrors.h"
#include "SolidSyslogPrival.h"
#include "ff.h"
}

using namespace CososoTesting;

static const char* const TEST_PATH = "test.log";

// Asserts the call raised exactly the File fault naming this operation, with
// the FRESULT behind it.
#define CHECK_FAILURE_REPORTED_WITH_RESULT(code, result) \
    CHECK_ERROR_REPORTED_ONCE_WITH_NATIVE_CODE(          \
        SOLIDSYSLOG_SEVERITY_ERROR,                      \
        &SolidSyslogFatFsFileErrorSource,                \
        SOLIDSYSLOG_CAT_FILE_IO_FAILED,                  \
        code,                                            \
        result                                           \
    )

// Asserts the call raised exactly the File fault naming this operation, with
// no code behind it.
#define CHECK_FAILURE_REPORTED_ALONE(code) \
    CHECK_ERROR_REPORTED_ONCE(             \
        SOLIDSYSLOG_SEVERITY_ERROR,        \
        &SolidSyslogFatFsFileErrorSource,  \
        SOLIDSYSLOG_CAT_FILE_IO_FAILED,    \
        code                               \
    )

#define CHECK_FILE_IS_OPEN() CHECK_TRUE(SolidSyslogFile_IsOpen(file))
#define CHECK_FILE_CLOSED() CHECK_FALSE(SolidSyslogFile_IsOpen(file))
#define CHECK_OPEN_PATH(path) STRCMP_EQUAL((path), FatFsFake_LastOpenPath())
#define CHECK_OPEN_MODE(mode) LONGS_EQUAL((mode), FatFsFake_LastOpenMode())
#define CHECK_LSEEK_OFFSET(offset) LONGS_EQUAL((offset), FatFsFake_LastLseekOffset())
#define CHECK_READ_COUNT(count) LONGS_EQUAL((count), FatFsFake_LastReadCount())
#define CHECK_WRITE_COUNT(count) LONGS_EQUAL((count), FatFsFake_LastWriteCount())
#define CHECK_STAT_PATH(path) STRCMP_EQUAL((path), FatFsFake_LastStatPath())
#define CHECK_UNLINK_PATH(path) STRCMP_EQUAL((path), FatFsFake_LastUnlinkPath())

// clang-format off
TEST_GROUP(SolidSyslogFatFsFile)
{
    struct SolidSyslogFile* file = nullptr;
    char buffer[5] = {'h', 'e', 'l', 'l', 'o'};

    void setup() override
    {
        FatFsFake_Reset();
        file = SolidSyslogFatFsFile_Create();
    }

    void teardown() override
    {
        SolidSyslogFatFsFile_Destroy(file);
    }

    void Open() const                 { CHECK_TRUE(SolidSyslogFile_Open(file, TEST_PATH)); }
    void Open(const char* path) const { CHECK_TRUE(SolidSyslogFile_Open(file, path)); }
    void Close() const                { SolidSyslogFile_Close(file); }
};

// clang-format on

TEST(SolidSyslogFatFsFile, CreateSucceeds)
{
    CHECK(file != nullptr);
}

TEST(SolidSyslogFatFsFile, IsOpenIsFalseAfterCreate)
{
    CHECK_FILE_CLOSED();
}

TEST(SolidSyslogFatFsFile, OpenSucceeds)
{
    CHECK_TRUE(SolidSyslogFile_Open(file, TEST_PATH));
    CHECK_FILE_IS_OPEN();
}

TEST(SolidSyslogFatFsFile, OpenCallsFOpenWithCorrectDefaults)
{
    Open();
    CALLED_FAKE(FatFsFake_Open, ONCE);
    CHECK_OPEN_PATH(TEST_PATH);
    CHECK_OPEN_MODE(FA_READ | FA_WRITE | FA_OPEN_ALWAYS);
}

TEST(SolidSyslogFatFsFile, OpenUsesPassedFilename)
{
    Open("different.log");
    CHECK_OPEN_PATH("different.log");
}

TEST(SolidSyslogFatFsFile, OpenFailsWhenFOpenFails)
{
    FatFsFake_SetOpenResult(FR_NO_PATH);
    CHECK_FALSE(SolidSyslogFile_Open(file, TEST_PATH));
    CHECK_FILE_CLOSED();
}

TEST(SolidSyslogFatFsFile, CloseCallsFCloseAndClearsIsOpen)
{
    Open();
    Close();
    CALLED_FAKE(FatFsFake_Close, ONCE);
    CHECK_FILE_CLOSED();
}

TEST(SolidSyslogFatFsFile, CloseIsNoOpWhenAlreadyClosed)
{
    Close();
    CALLED_FAKE(FatFsFake_Close, NEVER);
}

TEST(SolidSyslogFatFsFile, DestroyClosesOpenFile)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    SolidSyslogFatFsFile_Destroy(file);
    file = nullptr;
    CALLED_FAKE(FatFsFake_Close, ONCE);
}

TEST(SolidSyslogFatFsFile, TruncateSeeksToZeroAndCallsFTruncate)
{
    Open();
    SolidSyslogFile_Truncate(file);
    CALLED_FAKE(FatFsFake_Lseek, ONCE);
    CHECK_LSEEK_OFFSET(0);
    CALLED_FAKE(FatFsFake_Truncate, ONCE);
}

TEST(SolidSyslogFatFsFile, SeekToCallsFLseekWithGivenOffset)
{
    Open();
    SolidSyslogFile_SeekTo(file, 42);
    CALLED_FAKE(FatFsFake_Lseek, ONCE);
    CHECK_LSEEK_OFFSET(42);

    SolidSyslogFile_SeekTo(file, 99);
    CHECK_LSEEK_OFFSET(99);
}

TEST(SolidSyslogFatFsFile, SizeReturnsFileObjectSize)
{
    Open();
    FatFsFake_SetFileSize(42);
    CHECK_FILE_SIZE(file, 42);
}

TEST(SolidSyslogFatFsFile, ReadCallsFReadWithCorrectDefaults)
{
    Open();
    const char source[5] = {'h', 'e', 'l', 'l', 'o'};
    FatFsFake_SetReadSource(source, sizeof(source));
    CHECK_TRUE(SolidSyslogFile_Read(file, buffer, sizeof(buffer)));
    CALLED_FAKE(FatFsFake_Read, ONCE);
    MEMCMP_EQUAL(source, buffer, sizeof(source));
    CHECK_READ_COUNT(sizeof(buffer));
}

TEST(SolidSyslogFatFsFile, ReadFailsWhenFReadReturnsPartial)
{
    Open();
    FatFsFake_SetReadBytesReturned(3);
    CHECK_FALSE(SolidSyslogFile_Read(file, buffer, sizeof(buffer)));
}

TEST(SolidSyslogFatFsFile, ReadFailsWhenSourceShorterThanRequested)
{
    Open();
    const char source[3] = {'a', 'b', 'c'};
    FatFsFake_SetReadSource(source, sizeof(source));
    CHECK_FALSE(SolidSyslogFile_Read(file, buffer, sizeof(buffer)));
}

TEST(SolidSyslogFatFsFile, ReadFailsWhenFReadFails)
{
    Open();
    FatFsFake_SetReadResult(FR_DISK_ERR);
    CHECK_FALSE(SolidSyslogFile_Read(file, buffer, sizeof(buffer)));
}

TEST(SolidSyslogFatFsFile, WriteCallsFWriteWithCorrectDefaults)
{
    Open();
    CHECK_TRUE(SolidSyslogFile_Write(file, buffer, sizeof(buffer)));
    CALLED_FAKE(FatFsFake_Write, ONCE);
    MEMCMP_EQUAL(buffer, FatFsFake_LastWriteBytes(), sizeof(buffer));
    CHECK_WRITE_COUNT(sizeof(buffer));
}

TEST(SolidSyslogFatFsFile, WriteFailsWhenFWriteReturnsPartial)
{
    Open();
    FatFsFake_SetWriteBytesReturned(3);
    CHECK_FALSE(SolidSyslogFile_Write(file, buffer, sizeof(buffer)));
}

TEST(SolidSyslogFatFsFile, WriteFailsWhenFWriteFails)
{
    Open();
    FatFsFake_SetWriteResult(FR_DISK_ERR);
    CHECK_FALSE(SolidSyslogFile_Write(file, buffer, sizeof(buffer)));
}

TEST(SolidSyslogFatFsFile, WriteCommitsToDisk)
{
    Open();
    SolidSyslogFile_Write(file, buffer, 1);
    CALLED_FAKE(FatFsFake_Sync, ONCE);
}

TEST(SolidSyslogFatFsFile, WriteFailsWhenFSyncFails)
{
    Open();
    FatFsFake_SetSyncResult(FR_DISK_ERR);
    CHECK_FALSE(SolidSyslogFile_Write(file, buffer, sizeof(buffer)));
}

TEST(SolidSyslogFatFsFile, WriteDoesNotSyncWhenFWriteFails)
{
    Open();
    FatFsFake_SetWriteResult(FR_DISK_ERR);
    SolidSyslogFile_Write(file, buffer, sizeof(buffer));
    CALLED_FAKE(FatFsFake_Sync, NEVER);
}

TEST(SolidSyslogFatFsFile, ExistsCallsFStatAndReportsTrue)
{
    CHECK_TRUE(SolidSyslogFile_Exists(file, TEST_PATH));
    CALLED_FAKE(FatFsFake_Stat, ONCE);
    CHECK_STAT_PATH(TEST_PATH);
}

TEST(SolidSyslogFatFsFile, ExistsUsesPassedPath)
{
    SolidSyslogFile_Exists(file, "different.log");
    CHECK_STAT_PATH("different.log");
}

TEST(SolidSyslogFatFsFile, ExistsReportsFalseWhenFStatFails)
{
    FatFsFake_SetStatResult(FR_NO_FILE);
    CHECK_FALSE(SolidSyslogFile_Exists(file, TEST_PATH));
}

TEST(SolidSyslogFatFsFile, DeleteCallsFUnlinkAndReportsTrue)
{
    CHECK_TRUE(SolidSyslogFile_Delete(file, TEST_PATH));
    CALLED_FAKE(FatFsFake_Unlink, ONCE);
    CHECK_UNLINK_PATH(TEST_PATH);
}

TEST(SolidSyslogFatFsFile, DeleteUsesPassedPath)
{
    SolidSyslogFile_Delete(file, "different.log");
    CHECK_UNLINK_PATH("different.log");
}

TEST(SolidSyslogFatFsFile, DeleteReportsFalseWhenFUnlinkFails)
{
    FatFsFake_SetUnlinkResult(FR_DENIED);
    CHECK_FALSE(SolidSyslogFile_Delete(file, TEST_PATH));
}

TEST(SolidSyslogFatFsFile, OpenThatFailsReportsTheFailedOpenAndItsResult)
{
    ErrorHandlerFake_Install(nullptr);
    FatFsFake_SetOpenResult(FR_NO_PATH);

    SolidSyslogFile_Open(file, TEST_PATH);

    CHECK_FAILURE_REPORTED_WITH_RESULT(SOLIDSYSLOG_FILE_ERROR_OPEN_FAILED, FR_NO_PATH);
}

TEST(SolidSyslogFatFsFile, CloseThatFailsReportsTheFailedCloseAndItsResult)
{
    Open();
    ErrorHandlerFake_Install(nullptr);
    FatFsFake_SetCloseResult(FR_DISK_ERR);

    Close();

    CHECK_FAILURE_REPORTED_WITH_RESULT(SOLIDSYSLOG_FILE_ERROR_CLOSE_FAILED, FR_DISK_ERR);
}

TEST(SolidSyslogFatFsFile, CloseThatFailsStillLeavesTheFileClosed)
{
    Open();
    FatFsFake_SetCloseResult(FR_DISK_ERR);

    Close();

    CHECK_FILE_CLOSED();
}

TEST(SolidSyslogFatFsFile, ReadThatFailsReportsTheFailedReadAndItsResult)
{
    Open();
    ErrorHandlerFake_Install(nullptr);
    FatFsFake_SetReadResult(FR_DISK_ERR);

    SolidSyslogFile_Read(file, buffer, sizeof(buffer));

    CHECK_FAILURE_REPORTED_WITH_RESULT(SOLIDSYSLOG_FILE_ERROR_READ_FAILED, FR_DISK_ERR);
}

TEST(SolidSyslogFatFsFile, AShortReadReportsNothing)
{
    Open();
    ErrorHandlerFake_Install(nullptr);
    FatFsFake_SetReadBytesReturned(3);

    SolidSyslogFile_Read(file, buffer, sizeof(buffer));

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogFatFsFile, WriteThatFailsReportsTheFailedWriteAndItsResult)
{
    Open();
    ErrorHandlerFake_Install(nullptr);
    FatFsFake_SetWriteResult(FR_DISK_ERR);

    SolidSyslogFile_Write(file, buffer, sizeof(buffer));

    CHECK_FAILURE_REPORTED_WITH_RESULT(SOLIDSYSLOG_FILE_ERROR_WRITE_FAILED, FR_DISK_ERR);
}

TEST(SolidSyslogFatFsFile, AShortWriteReportsTheFailedWriteAlone)
{
    Open();
    ErrorHandlerFake_Install(nullptr);
    FatFsFake_SetWriteBytesReturned(3);

    SolidSyslogFile_Write(file, buffer, sizeof(buffer));

    CHECK_FAILURE_REPORTED_ALONE(SOLIDSYSLOG_FILE_ERROR_WRITE_FAILED);
}

TEST(SolidSyslogFatFsFile, SyncThatFailsReportsTheFailedSyncAndItsResult)
{
    Open();
    ErrorHandlerFake_Install(nullptr);
    FatFsFake_SetSyncResult(FR_DISK_ERR);

    SolidSyslogFile_Write(file, buffer, sizeof(buffer));

    CHECK_FAILURE_REPORTED_WITH_RESULT(SOLIDSYSLOG_FILE_ERROR_SYNC_FAILED, FR_DISK_ERR);
}

TEST(SolidSyslogFatFsFile, SeekToThatFailsReportsTheFailedSeekAndItsResult)
{
    Open();
    ErrorHandlerFake_Install(nullptr);
    FatFsFake_SetLseekResult(FR_DISK_ERR);

    SolidSyslogFile_SeekTo(file, 42);

    CHECK_FAILURE_REPORTED_WITH_RESULT(SOLIDSYSLOG_FILE_ERROR_SEEK_FAILED, FR_DISK_ERR);
}

TEST(SolidSyslogFatFsFile, TruncateThatFailsReportsTheFailedTruncateAndItsResult)
{
    Open();
    ErrorHandlerFake_Install(nullptr);
    FatFsFake_SetTruncateResult(FR_DENIED);

    SolidSyslogFile_Truncate(file);

    CHECK_FAILURE_REPORTED_WITH_RESULT(SOLIDSYSLOG_FILE_ERROR_TRUNCATE_FAILED, FR_DENIED);
}

TEST(SolidSyslogFatFsFile, TruncateWhoseSeekFailsReportsTheFailedTruncateAndTruncatesNothing)
{
    Open();
    ErrorHandlerFake_Install(nullptr);
    FatFsFake_SetLseekResult(FR_DISK_ERR);

    SolidSyslogFile_Truncate(file);

    CHECK_FAILURE_REPORTED_WITH_RESULT(SOLIDSYSLOG_FILE_ERROR_TRUNCATE_FAILED, FR_DISK_ERR);
    CALLED_FAKE(FatFsFake_Truncate, NEVER);
}

TEST(SolidSyslogFatFsFile, CallsThatSucceedReportNothing)
{
    ErrorHandlerFake_Install(nullptr);
    Open();

    size_t size = 0U;
    SolidSyslogFile_Write(file, buffer, sizeof(buffer));
    SolidSyslogFile_Size(file, &size);
    SolidSyslogFile_SeekTo(file, 0);
    SolidSyslogFile_Read(file, buffer, 0);
    SolidSyslogFile_Truncate(file);
    Close();

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogFatFsFile, TheErrorSourceNamesItsDetailsThroughFileErrors)
{
    CHECK(SolidSyslogFileErrors_Name == SolidSyslogFatFsFileErrorSource.DetailName);
}
