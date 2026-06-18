#include <stdexcept>

#include "FileFake.h"
#include "SolidSyslogFile.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(FileFake)
{
    struct FileFakeStorage storage = {};
    struct SolidSyslogFile* api = nullptr;

    void setup() override
    {
        api = FileFake_Create(&storage);
    }

    void teardown() override
    {
        FileFake_Destroy();
    }
};

// clang-format on

TEST(FileFake, CreateReturnsNonNull)
{
    CHECK_TRUE(api != nullptr);
}

TEST(FileFake, IsOpenReturnsFalseBeforeOpen)
{
    CHECK_FALSE(SolidSyslogFile_IsOpen(api));
}

TEST(FileFake, OpenSetsIsOpen)
{
    SolidSyslogFile_Open(api, "test.dat");
    CHECK_TRUE(SolidSyslogFile_IsOpen(api));
}

TEST(FileFake, CloseClearsIsOpen)
{
    SolidSyslogFile_Open(api, "test.dat");
    SolidSyslogFile_Close(api);
    CHECK_FALSE(SolidSyslogFile_IsOpen(api));
}

TEST(FileFake, WriteAndReadRoundTrip)
{
    SolidSyslogFile_Open(api, "test.dat");
    SolidSyslogFile_Write(api, "hello", 5);
    SolidSyslogFile_SeekTo(api, 0);

    char buf[16] = {};
    CHECK_TRUE(SolidSyslogFile_Read(api, buf, 5));
    MEMCMP_EQUAL("hello", buf, 5);
}

TEST(FileFake, ReadBeyondFileSizeFails)
{
    SolidSyslogFile_Open(api, "test.dat");

    char buf[16];
    CHECK_FALSE(SolidSyslogFile_Read(api, buf, 1));
}

TEST(FileFake, SeekToChangesReadPosition)
{
    SolidSyslogFile_Open(api, "test.dat");
    SolidSyslogFile_Write(api, "abcde", 5);
    SolidSyslogFile_SeekTo(api, 3);

    char buf[16] = {};
    SolidSyslogFile_Read(api, buf, 2);
    MEMCMP_EQUAL("de", buf, 2);
}

TEST(FileFake, SizeReturnsFileSize)
{
    SolidSyslogFile_Open(api, "test.dat");
    LONGS_EQUAL(0, SolidSyslogFile_Size(api));
    SolidSyslogFile_Write(api, "hello", 5);
    LONGS_EQUAL(5, SolidSyslogFile_Size(api));
}

TEST(FileFake, TruncateClearsContent)
{
    SolidSyslogFile_Open(api, "test.dat");
    SolidSyslogFile_Write(api, "hello", 5);
    SolidSyslogFile_Truncate(api);
    LONGS_EQUAL(0, SolidSyslogFile_Size(api));
}

TEST(FileFake, WriteAtPositionOverwritesContent)
{
    SolidSyslogFile_Open(api, "test.dat");
    SolidSyslogFile_Write(api, "hello", 5);
    SolidSyslogFile_SeekTo(api, 0);
    SolidSyslogFile_Write(api, "HE", 2);
    SolidSyslogFile_SeekTo(api, 0);

    char buf[16] = {};
    SolidSyslogFile_Read(api, buf, 5);
    MEMCMP_EQUAL("HEllo", buf, 5);
}

TEST(FileFake, FailNextOpenMakesOpenReturnFalse)
{
    FileFake_FailNextOpen(api);
    CHECK_FALSE(SolidSyslogFile_Open(api, "test.dat"));
    CHECK_FALSE(SolidSyslogFile_IsOpen(api));
}

TEST(FileFake, FailNextOpenOnlyAffectsOneCall)
{
    FileFake_FailNextOpen(api);
    CHECK_FALSE(SolidSyslogFile_Open(api, "test.dat"));
    CHECK_TRUE(SolidSyslogFile_Open(api, "test.dat"));
}

TEST(FileFake, FailNextWriteMakesWriteReturnFalse)
{
    SolidSyslogFile_Open(api, "test.dat");
    FileFake_FailNextWrite(api);
    CHECK_FALSE(SolidSyslogFile_Write(api, "hello", 5));
}

TEST(FileFake, FailNextWriteOnlyAffectsOneCall)
{
    SolidSyslogFile_Open(api, "test.dat");
    FileFake_FailNextWrite(api);
    CHECK_FALSE(SolidSyslogFile_Write(api, "hello", 5));
    CHECK_TRUE(SolidSyslogFile_Write(api, "world", 5));
}

TEST(FileFake, FailNextReadMakesReadReturnFalse)
{
    SolidSyslogFile_Open(api, "test.dat");
    SolidSyslogFile_Write(api, "hello", 5);
    SolidSyslogFile_SeekTo(api, 0);
    FileFake_FailNextRead(api);

    char buf[16];
    CHECK_FALSE(SolidSyslogFile_Read(api, buf, 5));
}

TEST(FileFake, FailNextReadOnlyAffectsOneCall)
{
    SolidSyslogFile_Open(api, "test.dat");
    SolidSyslogFile_Write(api, "hello", 5);
    SolidSyslogFile_SeekTo(api, 0);
    FileFake_FailNextRead(api);

    char buf[16] = {};
    CHECK_FALSE(SolidSyslogFile_Read(api, buf, 5));
    SolidSyslogFile_SeekTo(api, 0);
    CHECK_TRUE(SolidSyslogFile_Read(api, buf, 5));
}

TEST(FileFake, FailNextDeleteMakesDeleteReturnFalse)
{
    SolidSyslogFile_Open(api, "test.dat");
    SolidSyslogFile_Close(api);
    FileFake_FailNextDelete(api);
    CHECK_FALSE(SolidSyslogFile_Delete(api, "test.dat"));
    CHECK_TRUE(SolidSyslogFile_Exists(api, "test.dat"));
}

TEST(FileFake, FailNextDeleteOnlyAffectsOneCall)
{
    SolidSyslogFile_Open(api, "test.dat");
    SolidSyslogFile_Close(api);
    FileFake_FailNextDelete(api);
    CHECK_FALSE(SolidSyslogFile_Delete(api, "test.dat"));
    CHECK_TRUE(SolidSyslogFile_Delete(api, "test.dat"));
}

TEST(FileFake, FileContentReturnsInternalBuffer)
{
    SolidSyslogFile_Open(api, "test.dat");
    SolidSyslogFile_Write(api, "hello", 5);
    MEMCMP_EQUAL("hello", FileFake_FileContent(), 5);
}

TEST(FileFake, FileSizeMatchesApiSize)
{
    SolidSyslogFile_Open(api, "test.dat");
    SolidSyslogFile_Write(api, "hello", 5);
    LONGS_EQUAL(SolidSyslogFile_Size(api), FileFake_FileSize());
}

TEST(FileFake, OpenPreservesExistingContent)
{
    SolidSyslogFile_Open(api, "test.dat");
    SolidSyslogFile_Write(api, "hello", 5);
    SolidSyslogFile_Close(api);
    SolidSyslogFile_Open(api, "test.dat");

    LONGS_EQUAL(5, SolidSyslogFile_Size(api));
    char buf[16] = {};
    SolidSyslogFile_Read(api, buf, 5);
    MEMCMP_EQUAL("hello", buf, 5);
}

TEST(FileFake, ExistsReturnsTrueForOpenFile)
{
    SolidSyslogFile_Open(api, "test.dat");
    CHECK_TRUE(SolidSyslogFile_Exists(api, "test.dat"));
}

TEST(FileFake, ExistsReturnsFalseForUnknownFile)
{
    CHECK_FALSE(SolidSyslogFile_Exists(api, "unknown.dat"));
}

TEST(FileFake, ExistsReturnsTrueForClosedFile)
{
    SolidSyslogFile_Open(api, "test.dat");
    SolidSyslogFile_Close(api);
    CHECK_TRUE(SolidSyslogFile_Exists(api, "test.dat"));
}

TEST(FileFake, CanOpenTwoFilesByName)
{
    SolidSyslogFile_Open(api, "a.log");
    SolidSyslogFile_Write(api, "aaa", 3);
    SolidSyslogFile_Close(api);

    SolidSyslogFile_Open(api, "b.log");
    SolidSyslogFile_Write(api, "bbb", 3);
    SolidSyslogFile_Close(api);

    SolidSyslogFile_Open(api, "a.log");
    char buf[16] = {};
    SolidSyslogFile_Read(api, buf, 3);
    MEMCMP_EQUAL("aaa", buf, 3);
}

TEST(FileFake, ExistsDistinguishesFilesByName)
{
    SolidSyslogFile_Open(api, "a.log");
    SolidSyslogFile_Close(api);

    CHECK_TRUE(SolidSyslogFile_Exists(api, "a.log"));
    CHECK_FALSE(SolidSyslogFile_Exists(api, "b.log"));
}

TEST(FileFake, DeleteRemovesOnlyNamedFile)
{
    SolidSyslogFile_Open(api, "a.log");
    SolidSyslogFile_Close(api);
    SolidSyslogFile_Open(api, "b.log");
    SolidSyslogFile_Close(api);

    CHECK_TRUE(SolidSyslogFile_Delete(api, "a.log"));
    CHECK_FALSE(SolidSyslogFile_Exists(api, "a.log"));
    CHECK_TRUE(SolidSyslogFile_Exists(api, "b.log"));
}

TEST(FileFake, DeleteReturnsFalseForUnknownFile)
{
    CHECK_FALSE(SolidSyslogFile_Delete(api, "nonexistent.dat"));
}

TEST(FileFake, TwoInstancesShareFilesystem)
{
    SolidSyslogFile_Open(api, "shared.dat");
    SolidSyslogFile_Write(api, "hello", 5);
    /* Close before the second instance opens the same path — the S27.01
     * single-handle-per-path invariant forbids overlap. The point of this
     * test is the shared in-memory filesystem, not concurrent opens. */
    SolidSyslogFile_Close(api);

    struct FileFakeStorage storage2 = {};
    struct SolidSyslogFile* reader = FileFake_Create(&storage2);

    SolidSyslogFile_Open(reader, "shared.dat");
    char buf[16] = {};
    SolidSyslogFile_Read(reader, buf, 5);
    MEMCMP_EQUAL("hello", buf, 5);
    SolidSyslogFile_Close(reader);

    /* Restore lastCreated to group-owned storage so teardown doesn't use dangling pointer */
    api = FileFake_Create(&storage);
}

TEST(FileFake, OpenWhileAnotherInstanceHoldsPathOpenAsserts)
{
    SolidSyslogFile_Open(api, "shared.dat");

    struct FileFakeStorage storage2 = {};
    struct SolidSyslogFile* second = FileFake_Create(&storage2);

    CHECK_THROWS(std::runtime_error, SolidSyslogFile_Open(second, "shared.dat"));

    SolidSyslogFile_Close(api);
    api = FileFake_Create(&storage);
}

TEST(FileFake, CloseWithNoFileOpenThrows)
{
    CHECK_THROWS(std::runtime_error, SolidSyslogFile_Close(api));
}

TEST(FileFake, ReadWithNoFileOpenThrows)
{
    char buf[16];
    CHECK_THROWS(std::runtime_error, SolidSyslogFile_Read(api, buf, 1));
}

TEST(FileFake, WriteWithNoFileOpenThrows)
{
    CHECK_THROWS(std::runtime_error, SolidSyslogFile_Write(api, "x", 1));
}

TEST(FileFake, SeekToWithNoFileOpenThrows)
{
    CHECK_THROWS(std::runtime_error, SolidSyslogFile_SeekTo(api, 0));
}

TEST(FileFake, SizeWithNoFileOpenThrows)
{
    CHECK_THROWS(std::runtime_error, SolidSyslogFile_Size(api));
}

TEST(FileFake, TruncateWithNoFileOpenThrows)
{
    CHECK_THROWS(std::runtime_error, SolidSyslogFile_Truncate(api));
}

// clang-format off
TEST_GROUP(FileFakeAfterDestroy)
{
    struct FileFakeStorage storage = {};
    struct SolidSyslogFile* api = nullptr;

    void setup() override
    {
        api = FileFake_Create(&storage);
        FileFake_Destroy();
    }

    void teardown() override
    {
    }
};

// clang-format on

TEST(FileFakeAfterDestroy, OpenThrows)
{
    CHECK_THROWS(std::runtime_error, SolidSyslogFile_Open(api, "test.dat"));
}

TEST(FileFakeAfterDestroy, CloseThrows)
{
    CHECK_THROWS(std::runtime_error, SolidSyslogFile_Close(api));
}

TEST(FileFakeAfterDestroy, IsOpenThrows)
{
    CHECK_THROWS(std::runtime_error, SolidSyslogFile_IsOpen(api));
}

TEST(FileFakeAfterDestroy, ReadThrows)
{
    char buf[16];
    CHECK_THROWS(std::runtime_error, SolidSyslogFile_Read(api, buf, 1));
}

TEST(FileFakeAfterDestroy, WriteThrows)
{
    CHECK_THROWS(std::runtime_error, SolidSyslogFile_Write(api, "x", 1));
}

TEST(FileFakeAfterDestroy, SeekToThrows)
{
    CHECK_THROWS(std::runtime_error, SolidSyslogFile_SeekTo(api, 0));
}

TEST(FileFakeAfterDestroy, SizeThrows)
{
    CHECK_THROWS(std::runtime_error, SolidSyslogFile_Size(api));
}

TEST(FileFakeAfterDestroy, TruncateThrows)
{
    CHECK_THROWS(std::runtime_error, SolidSyslogFile_Truncate(api));
}

TEST(FileFakeAfterDestroy, ExistsThrows)
{
    CHECK_THROWS(std::runtime_error, SolidSyslogFile_Exists(api, "test.dat"));
}

TEST(FileFakeAfterDestroy, DeleteThrows)
{
    CHECK_THROWS(std::runtime_error, SolidSyslogFile_Delete(api, "test.dat"));
}

// clang-format off
TEST_GROUP(FileFakeStaleHandle)
{
    struct FileFakeStorage storageA = {};
    struct FileFakeStorage storageB = {};
    struct SolidSyslogFile* handleA = nullptr;
    struct SolidSyslogFile* handleB = nullptr;

    void setup() override
    {
        handleA = FileFake_Create(&storageA);
        handleB = FileFake_Create(&storageB);
    }

    void teardown() override
    {
        FileFake_Destroy();
    }
};

// clang-format on

TEST(FileFakeStaleHandle, ReadFailsOnStaleHandleAfterDeleteAndSlotReuse)
{
    SolidSyslogFile_Open(handleA, "old.dat");
    const char data[] = "original";
    SolidSyslogFile_Write(handleA, data, sizeof(data));

    SolidSyslogFile_Delete(handleB, "old.dat");

    /* Create a new file that could reuse the freed slot */
    SolidSyslogFile_Open(handleB, "new.dat");
    const char newData[] = "replaced";
    SolidSyslogFile_Write(handleB, newData, sizeof(newData));

    /* handleA still points at the old slot — read must fail, not return new file's data */
    SolidSyslogFile_SeekTo(handleA, 0);
    char buf[16] = {};
    bool success = SolidSyslogFile_Read(handleA, buf, sizeof(data));

    CHECK_FALSE(success);
}
