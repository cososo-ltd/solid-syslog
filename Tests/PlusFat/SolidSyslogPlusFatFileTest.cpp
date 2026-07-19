#include "CppUTest/TestHarness.h"
#include "TestUtils.h"

extern "C"
{
#include "PlusFatFake.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogPlusFatFile.h"
#include "ff_stdio.h"
}

using namespace CososoTesting;

static const char* const TEST_PATH = "test.log";

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

    LONGS_EQUAL(42, SolidSyslogFile_Size(file));
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
    PlusFatFake_SetRemoveFails();

    CHECK_FALSE(SolidSyslogFile_Delete(file, TEST_PATH));
}
