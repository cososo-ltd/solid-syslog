#include "CppUTest/TestHarness.h"

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWindowsFile.h"
#include "SolidSyslogWindowsFileErrors.h"
#include "TestUtils.h"

#include <cstdio>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

using namespace CososoTesting;

// Asserts handle is non-null and not one of the slots in pool.
#define CHECK_IS_FALLBACK(handle, pool)                                                \
    do                                                                                 \
    {                                                                                  \
        CHECK_TEXT((handle) != nullptr, "Fallback handle was nullptr");                \
        for (auto* slot : (pool))                                                      \
        {                                                                              \
            CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");   \
            CHECK_TEXT((handle) != slot, "Fallback handle collided with a pool slot"); \
        }                                                                              \
    } while (0)

// clang-format off

static std::string MakeTempPath(const char* filename)
{
    char  tempDir[MAX_PATH] = {};
    DWORD len               = GetTempPathA(MAX_PATH, tempDir);
    return std::string(tempDir, len) + filename;
}

TEST_GROUP(SolidSyslogWindowsFile)
{
    struct SolidSyslogFile* file = nullptr;
    std::string testPath;

    void setup() override
    {
        testPath = MakeTempPath("test_windows_file.dat");
        std::remove(testPath.c_str());
        file = SolidSyslogWindowsFile_Create();
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

// clang-format off
TEST_GROUP(SolidSyslogWindowsFilePool)
{
    struct SolidSyslogFile* pooled[SOLIDSYSLOG_FILE_POOL_SIZE] = {};
    struct SolidSyslogFile* overflow                                   = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogWindowsFile_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogWindowsFile_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogWindowsFile_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogWindowsFilePool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogWindowsFile_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogWindowsFilePool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogWindowsFile_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&WindowsFileErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_POOL_EXHAUSTED, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(WINDOWSFILE_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWindowsFilePool, FallbackOpenReturnsFalse)
{
    FillPool();
    overflow = SolidSyslogWindowsFile_Create();

    CHECK_FALSE(SolidSyslogFile_Open(overflow, "any_path"));
}

TEST(SolidSyslogWindowsFilePool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogWindowsFile_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWindowsFilePool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogWindowsFile_Create();

    LONGS_EQUAL(SOLIDSYSLOG_FILE_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_FILE_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogWindowsFilePool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogWindowsFile_Create();
    ConfigLockFake_Install();

    SolidSyslogWindowsFile_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWindowsFilePool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogFile stranger = {};

    SolidSyslogWindowsFile_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogWindowsFilePool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogFile stranger = {};

    SolidSyslogWindowsFile_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&WindowsFileErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_UNKNOWN_DESTROY, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(WINDOWSFILE_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWindowsFilePool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogWindowsFile_Create();
    SolidSyslogWindowsFile_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogWindowsFile_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&WindowsFileErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_UNKNOWN_DESTROY, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(WINDOWSFILE_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastDetail());
}
