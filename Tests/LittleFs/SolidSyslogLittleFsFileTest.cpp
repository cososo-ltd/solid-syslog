#include "CppUTest/TestHarness.h"
#include "TestUtils.h"

extern "C"
{
#include "ErrorHandlerFake.h"
#include "LittleFsFake.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFileErrors.h"
#include "SolidSyslogLittleFsFile.h"
#include "SolidSyslogLittleFsFileErrors.h"
#include "SolidSyslogNullFile.h"
#include "SolidSyslogPrival.h"
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

TEST(SolidSyslogLittleFsFile, TruncateEmptiesTheFileAndLeavesItOpen)
{
    CHECK_TRUE(SolidSyslogFile_Open(file, "test.log"));

    SolidSyslogFile_Truncate(file);

    LONGS_EQUAL(0, LittleFsFake_LastTruncateSize());
    LONGS_EQUAL(0, LittleFsFake_LastSeekOffset());
    CHECK_TRUE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogLittleFsFile, ExistsAsksTheFilesystemAboutThePath)
{
    CHECK_TRUE(SolidSyslogFile_Exists(file, "present.log"));
    STRCMP_EQUAL("present.log", LittleFsFake_LastStatPath());
}

TEST(SolidSyslogLittleFsFile, ExistsIsFalseWhenThePathIsAbsent)
{
    LittleFsFake_SetStatResult(LFS_ERR_NOENT);
    CHECK_FALSE(SolidSyslogFile_Exists(file, "absent.log"));
}

TEST(SolidSyslogLittleFsFile, DeleteRemovesThePath)
{
    CHECK_TRUE(SolidSyslogFile_Delete(file, "stale.log"));
    STRCMP_EQUAL("stale.log", LittleFsFake_LastRemovePath());
}

TEST(SolidSyslogLittleFsFile, DeleteSucceedsWhenThePathWasAlreadyAbsent)
{
    LittleFsFake_SetRemoveResult(LFS_ERR_NOENT);
    CHECK_TRUE(SolidSyslogFile_Delete(file, "gone.log"));
}

TEST(SolidSyslogLittleFsFile, DeleteFailsOnAnyOtherError)
{
    LittleFsFake_SetRemoveResult(LFS_ERR_IO);
    CHECK_FALSE(SolidSyslogFile_Delete(file, "locked.log"));
}

// clang-format off
TEST_GROUP(SolidSyslogLittleFsFileBadSetup)
{
    unsigned char fileBuffer[TEST_CACHE_SIZE] = {};

    void setup() override
    {
        LittleFsFake_Reset();
        ErrorHandlerFake_Install(nullptr);
    }

    // No teardown: a refused create holds no slot, and the handle it returns is
    // the shared NullFile, which is nobody's to destroy.
};

// clang-format on

TEST(SolidSyslogLittleFsFileBadSetup, NoFilesystemIsRefused)
{
    struct SolidSyslogFile* refused = SolidSyslogLittleFsFile_Create(nullptr, fileBuffer, sizeof(fileBuffer));

    POINTERS_EQUAL(SolidSyslogNullFile_Get(), refused);
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogLittleFsFileErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_FILE_ERROR_NULL_FILESYSTEM
    );
}

TEST(SolidSyslogLittleFsFileBadSetup, NoBufferIsRefused)
{
    lfs_t* filesystem = LittleFsFake_MountedWithCacheSize(TEST_CACHE_SIZE);

    struct SolidSyslogFile* refused = SolidSyslogLittleFsFile_Create(filesystem, nullptr, 0);

    POINTERS_EQUAL(SolidSyslogNullFile_Get(), refused);
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogLittleFsFileErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_FILE_ERROR_BUFFER_TOO_SMALL
    );
}

TEST(SolidSyslogLittleFsFileBadSetup, ABufferSmallerThanTheMountedCacheSizeIsRefused)
{
    lfs_t* filesystem = LittleFsFake_MountedWithCacheSize(TEST_CACHE_SIZE);

    struct SolidSyslogFile* refused = SolidSyslogLittleFsFile_Create(filesystem, fileBuffer, TEST_CACHE_SIZE - 1);

    POINTERS_EQUAL(SolidSyslogNullFile_Get(), refused);
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogLittleFsFileErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_FILE_ERROR_BUFFER_TOO_SMALL
    );
}

TEST(SolidSyslogLittleFsFileBadSetup, ABufferExactlyTheCacheSizeIsAccepted)
{
    lfs_t* filesystem = LittleFsFake_MountedWithCacheSize(TEST_CACHE_SIZE);

    struct SolidSyslogFile* accepted = SolidSyslogLittleFsFile_Create(filesystem, fileBuffer, TEST_CACHE_SIZE);

    CHECK_FALSE(SolidSyslogNullFile_Get() == accepted);
    SolidSyslogLittleFsFile_Destroy(accepted);
}

TEST(SolidSyslogLittleFsFileBadSetup, ABufferLargerThanTheCacheSizeIsAccepted)
{
    lfs_t* filesystem = LittleFsFake_MountedWithCacheSize(TEST_CACHE_SIZE - 1);

    struct SolidSyslogFile* accepted = SolidSyslogLittleFsFile_Create(filesystem, fileBuffer, TEST_CACHE_SIZE);

    CHECK_FALSE(SolidSyslogNullFile_Get() == accepted);
    SolidSyslogLittleFsFile_Destroy(accepted);
}

TEST(SolidSyslogLittleFsFile, DestroyClosesAFileLeftOpen)
{
    CHECK_TRUE(SolidSyslogFile_Open(file, "test.log"));

    SolidSyslogLittleFsFile_Destroy(file);

    LONGS_EQUAL(1, LittleFsFake_CloseCallCount());
    /* The group's teardown destroys again; the slot is already free and the
       handle already carries the Null vtable, so that is a safe no-op. */
    file = SolidSyslogNullFile_Get();
}
