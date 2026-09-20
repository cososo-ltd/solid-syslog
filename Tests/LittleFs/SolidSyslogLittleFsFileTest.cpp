#include "CppUTest/TestHarness.h"
#include "TestUtils.h"

extern "C"
{
#include "LittleFsFake.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogLittleFsFile.h"
#include "lfs.h"
}

using namespace CososoTesting;

static const lfs_size_t TEST_CACHE_SIZE = 64;

// clang-format off
TEST_GROUP(SolidSyslogLittleFsFile)
{
    struct SolidSyslogFile* file = nullptr;
    lfs_t* filesystem = nullptr;
    unsigned char fileBuffer[TEST_CACHE_SIZE] = {};

    void setup() override
    {
        LittleFsFake_Reset();
        filesystem = LittleFsFake_MountedWithCacheSize(TEST_CACHE_SIZE);
        file = SolidSyslogLittleFsFile_Create(filesystem, fileBuffer, sizeof(fileBuffer));
    }

    void teardown() override
    {
        SolidSyslogLittleFsFile_Destroy(file);
    }
};

// clang-format on

TEST(SolidSyslogLittleFsFile, IsOpenIsFalseAfterCreate)
{
    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogLittleFsFile, OpenSucceeds)
{
    CHECK_TRUE(SolidSyslogFile_Open(file, "test.log"));
    CHECK_TRUE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogLittleFsFile, OpenPassesPathFlagsAndTheCallersBuffer)
{
    CHECK_TRUE(SolidSyslogFile_Open(file, "records.log"));
    LONGS_EQUAL(1, LittleFsFake_OpenCallCount());
    STRCMP_EQUAL("records.log", LittleFsFake_LastOpenPath());
    LONGS_EQUAL(LFS_O_RDWR | LFS_O_CREAT, LittleFsFake_LastOpenFlags());
    POINTERS_EQUAL(fileBuffer, LittleFsFake_LastOpenBuffer());
}

TEST(SolidSyslogLittleFsFile, CloseClosesTheOpenFile)
{
    CHECK_TRUE(SolidSyslogFile_Open(file, "test.log"));
    SolidSyslogFile_Close(file);
    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
    LONGS_EQUAL(1, LittleFsFake_CloseCallCount());
}

TEST(SolidSyslogLittleFsFile, ReadReturnsTheBytesRequested)
{
    const unsigned char stored[] = {'a', 'b', 'c'};
    unsigned char into[3] = {};
    LittleFsFake_SetReadSource(stored, sizeof(stored));
    CHECK_TRUE(SolidSyslogFile_Open(file, "test.log"));

    CHECK_TRUE(SolidSyslogFile_Read(file, into, sizeof(into)));

    MEMCMP_EQUAL(stored, into, sizeof(stored));
}

TEST(SolidSyslogLittleFsFile, ReadFailsWhenFewerBytesAreAvailable)
{
    const unsigned char stored[] = {'a', 'b'};
    unsigned char into[3] = {};
    LittleFsFake_SetReadSource(stored, sizeof(stored));
    CHECK_TRUE(SolidSyslogFile_Open(file, "test.log"));

    CHECK_FALSE(SolidSyslogFile_Read(file, into, sizeof(into)));
}

TEST(SolidSyslogLittleFsFile, WriteCommitsBeforeReportingSuccess)
{
    const unsigned char record[] = {'r', 'e', 'c'};
    CHECK_TRUE(SolidSyslogFile_Open(file, "test.log"));

    CHECK_TRUE(SolidSyslogFile_Write(file, record, sizeof(record)));

    LONGS_EQUAL(sizeof(record), LittleFsFake_LastWriteCount());
    MEMCMP_EQUAL(record, LittleFsFake_LastWriteBytes(), sizeof(record));
    LONGS_EQUAL(1, LittleFsFake_SyncCallCount());
}

TEST(SolidSyslogLittleFsFile, WriteFailsWhenTheSyncFails)
{
    const unsigned char record[] = {'r', 'e', 'c'};
    LittleFsFake_SetSyncResult(LFS_ERR_IO);
    CHECK_TRUE(SolidSyslogFile_Open(file, "test.log"));

    CHECK_FALSE(SolidSyslogFile_Write(file, record, sizeof(record)));
}

TEST(SolidSyslogLittleFsFile, WriteFailsWhenFewerBytesAreAccepted)
{
    const unsigned char record[] = {'r', 'e', 'c'};
    LittleFsFake_SetWriteBytesAccepted(2);
    CHECK_TRUE(SolidSyslogFile_Open(file, "test.log"));

    CHECK_FALSE(SolidSyslogFile_Write(file, record, sizeof(record)));
    LONGS_EQUAL(0, LittleFsFake_SyncCallCount());
}

TEST(SolidSyslogLittleFsFile, SeekToPositionsFromTheStart)
{
    CHECK_TRUE(SolidSyslogFile_Open(file, "test.log"));

    SolidSyslogFile_SeekTo(file, 42);

    LONGS_EQUAL(42, LittleFsFake_LastSeekOffset());
    LONGS_EQUAL(LFS_SEEK_SET, LittleFsFake_LastSeekWhence());
}

TEST(SolidSyslogLittleFsFile, SizeReportsTheFileLength)
{
    LittleFsFake_SetFileSize(1234);
    CHECK_TRUE(SolidSyslogFile_Open(file, "test.log"));

    LONGS_EQUAL(1234, SolidSyslogFile_Size(file));
}

TEST(SolidSyslogLittleFsFile, SizeIsZeroWhenLittleFsReportsAnError)
{
    LittleFsFake_SetFileSizeError(LFS_ERR_IO);
    CHECK_TRUE(SolidSyslogFile_Open(file, "test.log"));

    LONGS_EQUAL(0, SolidSyslogFile_Size(file));
}
