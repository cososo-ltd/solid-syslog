#include "CppUTest/TestHarness.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogPosixFile.h"
#include "SocketFake.h"

#include <cstdio>

static const char* const TEST_PATH = "/tmp/test_posix_file.dat";

// clang-format off
TEST_GROUP(SolidSyslogPosixFile)
{
    SolidSyslogPosixFileStorage storage = {};
    struct SolidSyslogFile* file = nullptr;

    void setup() override
    {
        SocketFake_Reset();
        remove(TEST_PATH);
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        file = SolidSyslogPosixFile_Create(&storage);
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

TEST(SolidSyslogPosixFile, CreateReturnsHandleInsideCallerSuppliedStorage)
{
    SolidSyslogPosixFileStorage localStorage{};
    struct SolidSyslogFile*     localFile = SolidSyslogPosixFile_Create(&localStorage);
    POINTERS_EQUAL(&localStorage, localFile);
    SolidSyslogPosixFile_Destroy(localFile);
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
