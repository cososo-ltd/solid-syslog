#include "CppUTest/TestHarness.h"
#include "TestUtils.h"

extern "C"
{
#include "ErrorHandlerFake.h"
#include "PlusFatFake.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogFileCategories.h"
#include "SolidSyslogPlusFatFile.h"
#include "SolidSyslogFileErrors.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPlusFatFileErrors.h"
#include "SolidSyslogPrival.h"
#include "ff_stdio.h"
}

using namespace CososoTesting;

static const char* const TEST_PATH = "test.log";

// Asserts the call raised exactly the File fault naming this operation, with
// the code behind it: the task errno, or the FF_Error_t of the flush.
#define CHECK_FAILURE_REPORTED_WITH_CODE(code, nativeCode) \
    CHECK_ERROR_REPORTED_ONCE_WITH_NATIVE_CODE(            \
        SOLIDSYSLOG_SEVERITY_ERROR,                        \
        &SolidSyslogPlusFatFileErrorSource,                \
        SOLIDSYSLOG_CAT_FILE_IO_FAILED,                    \
        code,                                              \
        nativeCode                                         \
    )

// Asserts the call raised exactly the File fault naming this operation, with
// no code behind it.
#define CHECK_FAILURE_REPORTED_ALONE(code)  \
    CHECK_ERROR_REPORTED_ONCE(              \
        SOLIDSYSLOG_SEVERITY_ERROR,         \
        &SolidSyslogPlusFatFileErrorSource, \
        SOLIDSYSLOG_CAT_FILE_IO_FAILED,     \
        code                                \
    )

// clang-format off
TEST_GROUP(SolidSyslogPlusFatFile)
{
    struct SolidSyslogFile* file = nullptr;
    char buffer[5] = {};

    void setup() override
    {
        PlusFatFake_Reset();
        file = SolidSyslogPlusFatFile_Create();
    }

    void teardown() override
    {
        SolidSyslogPlusFatFile_Destroy(file);
    }
};

// clang-format on

TEST(SolidSyslogPlusFatFile, CreateSucceeds)
{
    CHECK(file != nullptr);
}

TEST(SolidSyslogPlusFatFile, OpenSucceeds)
{
    CHECK_TRUE(SolidSyslogFile_Open(file, TEST_PATH));
    CHECK_TRUE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogPlusFatFile, OpenFallsBackToCreateModeWhenFileAbsent)
{
    PlusFatFake_SetOpenFailsForMode("r+");
    PlusFatFake_SetOpenErrno(pdFREERTOS_ERRNO_ENOENT);

    CHECK_TRUE(SolidSyslogFile_Open(file, TEST_PATH));

    LONGS_EQUAL(2, PlusFatFake_OpenCallCount());
    STRCMP_EQUAL(TEST_PATH, PlusFatFake_LastOpenPath());
    STRCMP_EQUAL("r+", PlusFatFake_OpenModeAt(0));
    STRCMP_EQUAL("w+", PlusFatFake_OpenModeAt(1));
}

TEST(SolidSyslogPlusFatFile, OpenDoesNotTruncateWhenReadPlusFailsForANonAbsentReason)
{
    PlusFatFake_SetOpenFailsForMode("r+");
    PlusFatFake_SetOpenErrno(pdFREERTOS_ERRNO_EIO);

    CHECK_FALSE(SolidSyslogFile_Open(file, TEST_PATH));

    LONGS_EQUAL(1, PlusFatFake_OpenCallCount());
    STRCMP_EQUAL("r+", PlusFatFake_OpenModeAt(0));
    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogPlusFatFile, OpenFailsWhenFileAbsentAndCreateFails)
{
    PlusFatFake_SetOpenAlwaysFails();
    PlusFatFake_SetOpenErrno(pdFREERTOS_ERRNO_ENOENT);

    CHECK_FALSE(SolidSyslogFile_Open(file, TEST_PATH));

    LONGS_EQUAL(2, PlusFatFake_OpenCallCount());
    STRCMP_EQUAL("r+", PlusFatFake_OpenModeAt(0));
    STRCMP_EQUAL("w+", PlusFatFake_OpenModeAt(1));
    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogPlusFatFile, CloseCallsFfcloseAndClearsIsOpen)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_Close(file);

    CALLED_FAKE(PlusFatFake_Close, ONCE);
    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogPlusFatFile, CloseIsNoOpWhenAlreadyClosed)
{
    SolidSyslogFile_Close(file);

    CALLED_FAKE(PlusFatFake_Close, NEVER);
}

TEST(SolidSyslogPlusFatFile, DestroyClosesOpenFile)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogPlusFatFile_Destroy(file);
    file = nullptr;

    CALLED_FAKE(PlusFatFake_Close, ONCE);
}

TEST(SolidSyslogPlusFatFile, ReadReturnsRequestedBytes)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    const char source[5] = {'h', 'e', 'l', 'l', 'o'};
    PlusFatFake_SetReadSource(source, sizeof(source));

    CHECK_TRUE(SolidSyslogFile_Read(file, buffer, sizeof(buffer)));

    MEMCMP_EQUAL(source, buffer, sizeof(source));
}

TEST(SolidSyslogPlusFatFile, ReadCallsFfreadWithUnitSizeAndCount)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    const char source[5] = {'h', 'e', 'l', 'l', 'o'};
    PlusFatFake_SetReadSource(source, sizeof(source));

    SolidSyslogFile_Read(file, buffer, sizeof(buffer));

    CALLED_FAKE(PlusFatFake_Read, ONCE);
    UNSIGNED_LONGS_EQUAL(1, PlusFatFake_LastReadSize());
    UNSIGNED_LONGS_EQUAL(sizeof(buffer), PlusFatFake_LastReadItems());
}

TEST(SolidSyslogPlusFatFile, ReadFailsWhenFewerBytesAvailable)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    const char source[3] = {'a', 'b', 'c'};
    PlusFatFake_SetReadSource(source, sizeof(source));

    CHECK_FALSE(SolidSyslogFile_Read(file, buffer, sizeof(buffer)));
}

TEST(SolidSyslogPlusFatFile, WriteCallsFfwriteWithData)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    CHECK_TRUE(SolidSyslogFile_Write(file, buffer, sizeof(buffer)));

    CALLED_FAKE(PlusFatFake_Write, ONCE);
    MEMCMP_EQUAL(buffer, PlusFatFake_LastWriteBytes(), sizeof(buffer));
    UNSIGNED_LONGS_EQUAL(sizeof(buffer), PlusFatFake_LastWriteItems());
}

TEST(SolidSyslogPlusFatFile, WriteCommitsToDiskWithFlushCache)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_Write(file, buffer, 1);

    CALLED_FAKE(PlusFatFake_FlushCache, ONCE);
}

TEST(SolidSyslogPlusFatFile, WriteFailsAndSkipsFlushWhenFfwriteIncomplete)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    PlusFatFake_SetWriteIncomplete();

    CHECK_FALSE(SolidSyslogFile_Write(file, buffer, sizeof(buffer)));

    CALLED_FAKE(PlusFatFake_FlushCache, NEVER);
}

TEST(SolidSyslogPlusFatFile, WriteFailsWhenFlushCacheFails)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    PlusFatFake_SetFlushCacheFails();

    CHECK_FALSE(SolidSyslogFile_Write(file, buffer, sizeof(buffer)));
}

TEST(SolidSyslogPlusFatFile, SeekToCallsFfseekFromStart)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_SeekTo(file, 42);

    CALLED_FAKE(PlusFatFake_Seek, ONCE);
    LONGS_EQUAL(42, PlusFatFake_LastSeekOffset());
    LONGS_EQUAL(SEEK_SET, PlusFatFake_LastSeekWhence());
}

TEST(SolidSyslogPlusFatFile, SizeReturnsFfilelength)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    PlusFatFake_SetFileLength(42);

    CHECK_FILE_SIZE(file, 42);
}

TEST(SolidSyslogPlusFatFile, SizeFailsWhenFfilelengthLeavesAnErrno)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    PlusFatFake_SetFileLengthErrno(pdFREERTOS_ERRNO_EBADF);

    size_t size = 42U;
    CHECK_FALSE(SolidSyslogFile_Size(file, &size));
    LONGS_EQUAL(0, size);
}

TEST(SolidSyslogPlusFatFile, TruncateSeeksToZeroAndCallsFseteof)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_Truncate(file);

    CALLED_FAKE(PlusFatFake_Seek, ONCE);
    LONGS_EQUAL(0, PlusFatFake_LastSeekOffset());
    CALLED_FAKE(PlusFatFake_Seteof, ONCE);
}

TEST(SolidSyslogPlusFatFile, ExistsCallsFstatAndReportsTrue)
{
    CHECK_TRUE(SolidSyslogFile_Exists(file, TEST_PATH));

    CALLED_FAKE(PlusFatFake_Stat, ONCE);
    STRCMP_EQUAL(TEST_PATH, PlusFatFake_LastStatPath());
}

TEST(SolidSyslogPlusFatFile, ExistsReportsFalseWhenFstatFails)
{
    PlusFatFake_SetStatFails();

    CHECK_FALSE(SolidSyslogFile_Exists(file, TEST_PATH));
}

TEST(SolidSyslogPlusFatFile, DeleteCallsFremoveAndReportsTrue)
{
    CHECK_TRUE(SolidSyslogFile_Delete(file, TEST_PATH));

    CALLED_FAKE(PlusFatFake_Remove, ONCE);
    STRCMP_EQUAL(TEST_PATH, PlusFatFake_LastRemovePath());
}

TEST(SolidSyslogPlusFatFile, DeleteReportsFalseWhenFremoveFails)
{
    PlusFatFake_SetRemoveErrno(pdFREERTOS_ERRNO_EACCES);

    CHECK_FALSE(SolidSyslogFile_Delete(file, TEST_PATH));
}

TEST(SolidSyslogPlusFatFile, DeleteReportsTrueWhenThePathIsAlreadyAbsent)
{
    PlusFatFake_SetRemoveErrno(pdFREERTOS_ERRNO_ENOENT);

    CHECK_TRUE(SolidSyslogFile_Delete(file, TEST_PATH));
}

TEST(SolidSyslogPlusFatFile, OpenThatFailsForANonAbsentReasonReportsTheFailedOpenAndItsErrno)
{
    ErrorHandlerFake_Install(nullptr);
    PlusFatFake_SetOpenFailsForMode("r+");
    PlusFatFake_SetOpenErrno(pdFREERTOS_ERRNO_EIO);

    SolidSyslogFile_Open(file, TEST_PATH);

    CHECK_FAILURE_REPORTED_WITH_CODE(SOLIDSYSLOG_FILE_ERROR_OPEN_FAILED, pdFREERTOS_ERRNO_EIO);
}

TEST(SolidSyslogPlusFatFile, OpenWhoseCreateFailsReportsTheFailedOpenAndItsErrno)
{
    ErrorHandlerFake_Install(nullptr);
    PlusFatFake_SetOpenAlwaysFails();
    PlusFatFake_SetOpenErrno(pdFREERTOS_ERRNO_ENOENT);

    SolidSyslogFile_Open(file, TEST_PATH);

    CHECK_FAILURE_REPORTED_WITH_CODE(SOLIDSYSLOG_FILE_ERROR_OPEN_FAILED, pdFREERTOS_ERRNO_ENOENT);
}

TEST(SolidSyslogPlusFatFile, AnOpenThatCreatesTheFileReportsNothing)
{
    ErrorHandlerFake_Install(nullptr);
    PlusFatFake_SetOpenFailsForMode("r+");
    PlusFatFake_SetOpenErrno(pdFREERTOS_ERRNO_ENOENT);

    SolidSyslogFile_Open(file, TEST_PATH);

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogPlusFatFile, CloseThatFailsReportsTheFailedCloseAndItsErrno)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    ErrorHandlerFake_Install(nullptr);
    PlusFatFake_SetCloseErrno(pdFREERTOS_ERRNO_EIO);

    SolidSyslogFile_Close(file);

    CHECK_FAILURE_REPORTED_WITH_CODE(SOLIDSYSLOG_FILE_ERROR_CLOSE_FAILED, pdFREERTOS_ERRNO_EIO);
}

TEST(SolidSyslogPlusFatFile, CloseThatFailsStillLeavesTheFileClosed)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    PlusFatFake_SetCloseErrno(pdFREERTOS_ERRNO_EIO);

    SolidSyslogFile_Close(file);

    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogPlusFatFile, ReadThatFailsReportsTheFailedReadAndItsErrno)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    ErrorHandlerFake_Install(nullptr);
    PlusFatFake_SetReadErrno(pdFREERTOS_ERRNO_EIO);

    SolidSyslogFile_Read(file, buffer, sizeof(buffer));

    CHECK_FAILURE_REPORTED_WITH_CODE(SOLIDSYSLOG_FILE_ERROR_READ_FAILED, pdFREERTOS_ERRNO_EIO);
}

TEST(SolidSyslogPlusFatFile, AShortReadReportsNothing)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    const char source[3] = {'a', 'b', 'c'};
    PlusFatFake_SetReadSource(source, sizeof(source));
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogFile_Read(file, buffer, sizeof(buffer));

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogPlusFatFile, WriteThatFailsReportsTheFailedWriteAndItsErrno)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    ErrorHandlerFake_Install(nullptr);
    PlusFatFake_SetWriteErrno(pdFREERTOS_ERRNO_ENOSPC);

    SolidSyslogFile_Write(file, buffer, sizeof(buffer));

    CHECK_FAILURE_REPORTED_WITH_CODE(SOLIDSYSLOG_FILE_ERROR_WRITE_FAILED, pdFREERTOS_ERRNO_ENOSPC);
}

TEST(SolidSyslogPlusFatFile, AnIncompleteWriteReportsTheFailedWriteAlone)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    ErrorHandlerFake_Install(nullptr);
    PlusFatFake_SetWriteIncomplete();

    SolidSyslogFile_Write(file, buffer, sizeof(buffer));

    CHECK_FAILURE_REPORTED_ALONE(SOLIDSYSLOG_FILE_ERROR_WRITE_FAILED);
}

TEST(SolidSyslogPlusFatFile, FlushThatFailsReportsTheFailedSyncAndItsCode)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    ErrorHandlerFake_Install(nullptr);
    PlusFatFake_SetFlushCacheFails();

    SolidSyslogFile_Write(file, buffer, sizeof(buffer));

    CHECK_FAILURE_REPORTED_WITH_CODE(SOLIDSYSLOG_FILE_ERROR_SYNC_FAILED, PlusFatFake_FlushCacheFailureCode());
}

TEST(SolidSyslogPlusFatFile, SeekToThatFailsReportsTheFailedSeekAndItsErrno)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    ErrorHandlerFake_Install(nullptr);
    PlusFatFake_SetSeekErrno(pdFREERTOS_ERRNO_EINVAL);

    SolidSyslogFile_SeekTo(file, 42);

    CHECK_FAILURE_REPORTED_WITH_CODE(SOLIDSYSLOG_FILE_ERROR_SEEK_FAILED, pdFREERTOS_ERRNO_EINVAL);
}

TEST(SolidSyslogPlusFatFile, SizeThatFailsReportsTheFailedSizeAndItsErrno)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    ErrorHandlerFake_Install(nullptr);
    PlusFatFake_SetFileLengthErrno(pdFREERTOS_ERRNO_EBADF);

    size_t size = 0U;
    SolidSyslogFile_Size(file, &size);

    CHECK_FAILURE_REPORTED_WITH_CODE(SOLIDSYSLOG_FILE_ERROR_SIZE_FAILED, pdFREERTOS_ERRNO_EBADF);
}

TEST(SolidSyslogPlusFatFile, TruncateThatFailsReportsTheFailedTruncateAndItsErrno)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    ErrorHandlerFake_Install(nullptr);
    PlusFatFake_SetSeteofErrno(pdFREERTOS_ERRNO_EIO);

    SolidSyslogFile_Truncate(file);

    CHECK_FAILURE_REPORTED_WITH_CODE(SOLIDSYSLOG_FILE_ERROR_TRUNCATE_FAILED, pdFREERTOS_ERRNO_EIO);
}

TEST(SolidSyslogPlusFatFile, TruncateWhoseSeekFailsReportsTheFailedTruncateAndTruncatesNothing)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    ErrorHandlerFake_Install(nullptr);
    PlusFatFake_SetSeekErrno(pdFREERTOS_ERRNO_EINVAL);

    SolidSyslogFile_Truncate(file);

    CHECK_FAILURE_REPORTED_WITH_CODE(SOLIDSYSLOG_FILE_ERROR_TRUNCATE_FAILED, pdFREERTOS_ERRNO_EINVAL);
    CALLED_FAKE(PlusFatFake_Seteof, NEVER);
}

TEST(SolidSyslogPlusFatFile, CallsThatSucceedReportNothing)
{
    ErrorHandlerFake_Install(nullptr);
    SolidSyslogFile_Open(file, TEST_PATH);
    PlusFatFake_SetReadSource(buffer, sizeof(buffer));

    size_t size = 0U;
    SolidSyslogFile_Write(file, buffer, sizeof(buffer));
    SolidSyslogFile_Size(file, &size);
    SolidSyslogFile_SeekTo(file, 0);
    SolidSyslogFile_Read(file, buffer, sizeof(buffer));
    SolidSyslogFile_Truncate(file);
    SolidSyslogFile_Close(file);

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogPlusFatFile, TheErrorSourceNamesItsDetailsThroughFileErrors)
{
    CHECK(SolidSyslogFileErrors_Name == SolidSyslogPlusFatFileErrorSource.DetailName);
}
