#include <cstddef>
#include <cstdint>
#include <cstring>

#include "CppUTest/TestHarness.h"

#include "ErrorHandlerFake.h"
#include "RamFileFake.h"
#include "SolidSyslogError.h"
#include "SolidSyslogFileCategories.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogFileErrors.h"
#include "SolidSyslogPrival.h"
#include "TestUtils.h"

using namespace CososoTesting;

// clang-format off
TEST_GROUP(RamFileFake)
{
    struct SolidSyslogFile* file = nullptr;
    char read[8] = {};

    void setup() override
    {
        file = RamFileFake_Create();
        ErrorHandlerFake_Install(nullptr);
    }

    void teardown() override
    {
        SolidSyslog_SetErrorHandler(nullptr, nullptr);
        RamFileFake_Destroy(file);
    }

    [[nodiscard]] bool Open(const char* path) const
    {
        return file->Open(file, path);
    }

    [[nodiscard]] bool Write(const char* text) const
    {
        return file->Write(file, text, strlen(text));
    }

    [[nodiscard]] bool Read(size_t count)
    {
        return file->Read(file, read, count);
    }

    [[nodiscard]] size_t Size() const
    {
        size_t size = 99U;
        (void) file->Size(file, &size);
        return size;
    }

    void Reopen(const char* path) const
    {
        file->Close(file);
        (void) file->Open(file, path);
    }
};

// clang-format on

TEST(RamFileFake, OpeningAPathCreatesItEmpty)
{
    CHECK_TRUE(Open("a"));
    LONGS_EQUAL(0, Size());
}

TEST(RamFileFake, AnOpenFileIsOpen)
{
    (void) Open("a");
    CHECK_TRUE(file->IsOpen(file));
}

TEST(RamFileFake, AClosedFileIsNotOpen)
{
    (void) Open("a");
    file->Close(file);
    CHECK_FALSE(file->IsOpen(file));
}

TEST(RamFileFake, WhatIsWrittenIsReadBack)
{
    (void) Open("a");
    CHECK_TRUE(Write("abc"));
    file->SeekTo(file, 0U);
    CHECK_TRUE(Read(3U));
    MEMCMP_EQUAL("abc", read, 3U);
}

TEST(RamFileFake, AWriteAtAPositionOverwritesThere)
{
    (void) Open("a");
    (void) Write("abc");
    file->SeekTo(file, 1U);
    (void) Write("X");
    file->SeekTo(file, 0U);
    (void) Read(3U);
    MEMCMP_EQUAL("aXc", read, 3U);
}

TEST(RamFileFake, AReadPastTheEndFails)
{
    (void) Open("a");
    (void) Write("ab");
    file->SeekTo(file, 1U);
    CHECK_FALSE(Read(2U));
}

TEST(RamFileFake, TheSizeIsTheBytesWritten)
{
    (void) Open("a");
    (void) Write("abc");
    LONGS_EQUAL(3, Size());
}

TEST(RamFileFake, ReopeningKeepsTheContent)
{
    (void) Open("a");
    (void) Write("abc");
    Reopen("a");
    LONGS_EQUAL(3, Size());
}

TEST(RamFileFake, EachPathHasItsOwnContent)
{
    (void) Open("a");
    (void) Write("abc");
    Reopen("b");
    LONGS_EQUAL(0, Size());
}

TEST(RamFileFake, TruncatingEmptiesTheFile)
{
    (void) Open("a");
    (void) Write("abc");
    file->Truncate(file);
    LONGS_EQUAL(0, Size());
}

TEST(RamFileFake, APathOpenedExists)
{
    (void) Open("a");
    CHECK_TRUE(file->Exists(file, "a"));
}

TEST(RamFileFake, APathNeverOpenedDoesNotExist)
{
    CHECK_FALSE(file->Exists(file, "a"));
}

TEST(RamFileFake, ADeletedPathNoLongerExists)
{
    (void) Open("a");
    file->Close(file);
    CHECK_TRUE(file->Delete(file, "a"));
    CHECK_FALSE(file->Exists(file, "a"));
}

TEST(RamFileFake, DeletingAPathThatIsAbsentSucceeds)
{
    CHECK_TRUE(file->Delete(file, "a"));
}

TEST(RamFileFake, AWorkingFileReportsNothing)
{
    (void) Open("a");
    (void) Write("abc");
    file->SeekTo(file, 0U);
    (void) Read(3U);

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(RamFileFake, AFailedFileFailsAnOpen)
{
    RamFileFake_FailEveryOperation(file, 5);
    CHECK_FALSE(Open("a"));
}

TEST(RamFileFake, AFailedOpenIsReportedWithItsErrno)
{
    RamFileFake_FailEveryOperation(file, 5);

    (void) Open("a");

    CHECK_ERROR_REPORTED_ONCE_WITH_NATIVE_CODE(
        SOLIDSYSLOG_FILE_IO_FAILED_SEVERITY,
        &RamFileFakeErrorSource,
        SOLIDSYSLOG_CAT_FILE_IO_FAILED,
        SOLIDSYSLOG_FILE_ERROR_OPEN_FAILED,
        5
    );
}

TEST(RamFileFake, AFailedFileFailsAWriteAndNamesIt)
{
    (void) Open("a");
    RamFileFake_FailEveryOperation(file, 5);

    CHECK_FALSE(Write("abc"));

    LONGS_EQUAL(SOLIDSYSLOG_FILE_ERROR_WRITE_FAILED, ErrorHandlerFake_LastDetail());
}

TEST(RamFileFake, AFailedFileFailsAReadAndNamesIt)
{
    (void) Open("a");
    (void) Write("abc");
    file->SeekTo(file, 0U);
    RamFileFake_FailEveryOperation(file, 5);

    CHECK_FALSE(Read(3U));

    LONGS_EQUAL(SOLIDSYSLOG_FILE_ERROR_READ_FAILED, ErrorHandlerFake_LastDetail());
}

TEST(RamFileFake, AFailedFileFailsASizeAndNamesIt)
{
    (void) Open("a");
    RamFileFake_FailEveryOperation(file, 5);
    size_t size = 99U;

    CHECK_FALSE(file->Size(file, &size));

    LONGS_EQUAL(0, size);
    LONGS_EQUAL(SOLIDSYSLOG_FILE_ERROR_SIZE_FAILED, ErrorHandlerFake_LastDetail());
}

TEST(RamFileFake, AFailedFileReportsASeekAndATruncate)
{
    (void) Open("a");
    RamFileFake_FailEveryOperation(file, 5);

    file->SeekTo(file, 0U);
    LONGS_EQUAL(SOLIDSYSLOG_FILE_ERROR_SEEK_FAILED, ErrorHandlerFake_LastDetail());
    file->Truncate(file);
    LONGS_EQUAL(SOLIDSYSLOG_FILE_ERROR_TRUNCATE_FAILED, ErrorHandlerFake_LastDetail());
}

TEST(RamFileFake, TheSourceNamesTheSharedFileDetails)
{
    STRCMP_EQUAL("RamFileFake", RamFileFakeErrorSource.Name);
    STRCMP_EQUAL("OPEN_FAILED", RamFileFakeErrorSource.DetailName(SOLIDSYSLOG_FILE_ERROR_OPEN_FAILED));
}
