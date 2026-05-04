#include "CppUTest/TestHarness.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogWindowsFile.h"

#include <cstdio>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// clang-format off

static std::string MakeTempPath(const char* filename)
{
    char  tempDir[MAX_PATH] = {};
    DWORD len               = GetTempPathA(MAX_PATH, tempDir);
    return std::string(tempDir, len) + filename;
}

TEST_GROUP(SolidSyslogWindowsFile)
{
    SolidSyslogWindowsFileStorage storage = {};
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogFile* file = nullptr;
    std::string testPath;

    void setup() override
    {
        testPath = MakeTempPath("test_windows_file.dat");
        std::remove(testPath.c_str());
        // cppcheck-suppress unreadVariable -- used in tests; cppcheck does not model CppUTest macros
        file = SolidSyslogWindowsFile_Create(&storage);
    }

    void teardown() override
    {
        SolidSyslogWindowsFile_Destroy(file);
        std::remove(testPath.c_str());
    }

    void OpenTestFile() const
    {
        CHECK_TRUE(SolidSyslogFile_Open(file, testPath.c_str()));
    }
};

// clang-format on

TEST(SolidSyslogWindowsFile, CreateReturnsNonNull)
{
    CHECK_TRUE(file != nullptr);
}

TEST(SolidSyslogWindowsFile, CreateReturnsHandleInsideCallerSuppliedStorage)
{
    SolidSyslogWindowsFileStorage localStorage{};
    struct SolidSyslogFile*       localFile = SolidSyslogWindowsFile_Create(&localStorage);
    POINTERS_EQUAL(&localStorage, localFile);
    SolidSyslogWindowsFile_Destroy(localFile);
}

TEST(SolidSyslogWindowsFile, IsOpenReturnsFalseBeforeOpen)
{
    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogWindowsFile, OpenReturnsTrue)
{
    OpenTestFile();
}

TEST(SolidSyslogWindowsFile, OpenSetsIsOpen)
{
    OpenTestFile();
    CHECK_TRUE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogWindowsFile, CloseResetsIsOpen)
{
    OpenTestFile();
    SolidSyslogFile_Close(file);
    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogWindowsFile, WriteAndReadRoundTrip)
{
    OpenTestFile();
    CHECK_TRUE(SolidSyslogFile_Write(file, "hello", 5));
    SolidSyslogFile_SeekTo(file, 0);

    char buf[16] = {};
    CHECK_TRUE(SolidSyslogFile_Read(file, buf, 5));
    MEMCMP_EQUAL("hello", buf, 5);
}

TEST(SolidSyslogWindowsFile, SizeReturnsFileSize)
{
    OpenTestFile();
    LONGS_EQUAL(0, SolidSyslogFile_Size(file));
    CHECK_TRUE(SolidSyslogFile_Write(file, "hello", 5));
    LONGS_EQUAL(5, SolidSyslogFile_Size(file));
}

TEST(SolidSyslogWindowsFile, TruncateClearsFile)
{
    OpenTestFile();
    CHECK_TRUE(SolidSyslogFile_Write(file, "hello", 5));
    SolidSyslogFile_Truncate(file);
    LONGS_EQUAL(0, SolidSyslogFile_Size(file));
}

TEST(SolidSyslogWindowsFile, BinaryRoundTripPreservesNewlineBytes)
{
    /* Without _O_BINARY the MSVC CRT would substitute 0x0D 0x0A for 0x0A on
     * write and strip 0x0D on read. This test catches a regression where the
     * flag is dropped or replaced with _O_TEXT. */
    OpenTestFile();
    const unsigned char payload[] = {0x01, 0x0A, 0x0D, 0x0A, 0xFF};
    CHECK_TRUE(SolidSyslogFile_Write(file, payload, sizeof(payload)));
    SolidSyslogFile_SeekTo(file, 0);

    unsigned char roundTrip[sizeof(payload)] = {};
    CHECK_TRUE(SolidSyslogFile_Read(file, roundTrip, sizeof(roundTrip)));
    MEMCMP_EQUAL(payload, roundTrip, sizeof(payload));
    LONGS_EQUAL(sizeof(payload), SolidSyslogFile_Size(file));
}

TEST(SolidSyslogWindowsFile, OpenWithInvalidPathReturnsFalse)
{
    CHECK_FALSE(SolidSyslogFile_Open(file, "C:\\nonexistent_dir_solidsyslog_test\\file.dat"));
}

TEST(SolidSyslogWindowsFile, DeleteRemovesFile)
{
    OpenTestFile();
    SolidSyslogFile_Close(file);
    CHECK_TRUE(SolidSyslogFile_Delete(file, testPath.c_str()));
    CHECK_FALSE(SolidSyslogFile_Exists(file, testPath.c_str()));
}

TEST(SolidSyslogWindowsFile, DeleteReturnsFalseForNonexistentFile)
{
    CHECK_FALSE(SolidSyslogFile_Delete(file, MakeTempPath("nonexistent_windows_file.dat").c_str()));
}
