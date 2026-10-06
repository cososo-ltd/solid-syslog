#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogFileErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogVxWorks64File.h"
#include "SolidSyslogVxWorks64FileErrors.h"
#include "VxWorks64IoFake.h"

static const char* const TEST_PATH = "/ata0a/STORE00.LOG";
static const int ERROR_RESULT = -1;

// The VxWorks 6.4 values. A C++ test cannot include ioLib.h, whose renames are
// for the C sources it stands in for.
static const int VX_O_RDONLY = 0;
static const int VX_O_RDWR_CREAT = 0x0202;
static const int VX_SEEK_SET = 0;
static const int VX_SEEK_END = 2;
static const int VX_FIOSYNC = 21;
static const int VX_FIOTRUNC = 42;
static const int VX_FIOCOMMITFS = 56;
static const int OWNER_READ_WRITE = 0x180;

// Asserts handle is non-null and not one of the slots in pool.
#define CHECK_IS_FALLBACK(handle, pool)                                                \
    {                                                                                  \
        CHECK_TEXT((handle) != nullptr, "Fallback handle was nullptr");                \
        for (auto* slot : (pool))                                                      \
        {                                                                              \
            CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");   \
            CHECK_TEXT((handle) != slot, "Fallback handle collided with a pool slot"); \
        }                                                                              \
    }

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64File)
{
    struct SolidSyslogFile* file = nullptr;

    void setup() override
    {
        VxWorks64IoFake_Reset();
        file = SolidSyslogVxWorks64File_Create();
    }

    void teardown() override
    {
        SolidSyslogVxWorks64File_Destroy(file);
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64File, IsOpenIsFalseAfterCreate)
{
    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogVxWorks64File, OpenOpensThePathForReadingAndWritingCreatingIt)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    CALLED_FAKE(VxWorks64IoFake_Open, ONCE);
    STRCMP_EQUAL(TEST_PATH, VxWorks64IoFake_LastOpenName());
    LONGS_EQUAL(VX_O_RDWR_CREAT, VxWorks64IoFake_LastOpenFlags());
}

TEST(SolidSyslogVxWorks64File, OpenCreatesTheFileReadableAndWritableByItsOwner)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    LONGS_EQUAL(OWNER_READ_WRITE, VxWorks64IoFake_LastOpenMode());
}

TEST(SolidSyslogVxWorks64File, OpenReturnsTrueWhenOpenSucceeds)
{
    CHECK_TRUE(SolidSyslogFile_Open(file, TEST_PATH));
}

TEST(SolidSyslogVxWorks64File, OpenLeavesTheFileOpen)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    CHECK_TRUE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogVxWorks64File, OpenReturnsFalseWhenOpenFails)
{
    VxWorks64IoFake_FailOpens();

    CHECK_FALSE(SolidSyslogFile_Open(file, TEST_PATH));
}

TEST(SolidSyslogVxWorks64File, FileStaysClosedWhenOpenFails)
{
    VxWorks64IoFake_FailOpens();

    SolidSyslogFile_Open(file, TEST_PATH);

    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogVxWorks64File, CloseClosesTheDescriptorOpenReturned)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_Close(file);

    CALLED_FAKE(VxWorks64IoFake_Close, ONCE);
    LONGS_EQUAL(VxWorks64IoFake_Fd(), VxWorks64IoFake_LastClosedFd());
}

TEST(SolidSyslogVxWorks64File, CloseLeavesTheFileClosed)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_Close(file);

    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogVxWorks64File, CloseOfAClosedFileClosesNothing)
{
    SolidSyslogFile_Close(file);

    CALLED_FAKE(VxWorks64IoFake_Close, NEVER);
}

TEST(SolidSyslogVxWorks64File, DestroyClosesAnOpenFile)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogVxWorks64File_Destroy(file);
    file = nullptr;

    CALLED_FAKE(VxWorks64IoFake_Close, ONCE);
}

TEST(SolidSyslogVxWorks64File, ReadReadsFromTheOpenDescriptor)
{
    char buffer[5] = {};
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_Read(file, buffer, sizeof(buffer));

    CALLED_FAKE(VxWorks64IoFake_Read, ONCE);
    LONGS_EQUAL(VxWorks64IoFake_Fd(), VxWorks64IoFake_LastReadFd());
}

TEST(SolidSyslogVxWorks64File, ReadAsksForTheWholeCount)
{
    char buffer[5] = {};
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_Read(file, buffer, sizeof(buffer));

    LONGS_EQUAL(5, VxWorks64IoFake_LastReadMaxBytes());
}

TEST(SolidSyslogVxWorks64File, ReadDeliversTheBytesIntoTheBuffer)
{
    char buffer[5] = {};
    VxWorks64IoFake_ReadDelivers("hello", 5);
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_Read(file, buffer, sizeof(buffer));

    MEMCMP_EQUAL("hello", buffer, sizeof(buffer));
}

TEST(SolidSyslogVxWorks64File, ReadReturnsTrueWhenTheWholeCountArrives)
{
    char buffer[5] = {};
    VxWorks64IoFake_ReadDelivers("hello", 5);
    SolidSyslogFile_Open(file, TEST_PATH);

    CHECK_TRUE(SolidSyslogFile_Read(file, buffer, sizeof(buffer)));
}

TEST(SolidSyslogVxWorks64File, ReadReturnsFalseOnAShortRead)
{
    char buffer[5] = {};
    VxWorks64IoFake_ReadDelivers("hel", 3);
    SolidSyslogFile_Open(file, TEST_PATH);

    CHECK_FALSE(SolidSyslogFile_Read(file, buffer, sizeof(buffer)));
}

TEST(SolidSyslogVxWorks64File, ReadReturnsFalseWhenReadFails)
{
    char buffer[5] = {};
    VxWorks64IoFake_FailReads();
    SolidSyslogFile_Open(file, TEST_PATH);

    CHECK_FALSE(SolidSyslogFile_Read(file, buffer, sizeof(buffer)));
}

TEST(SolidSyslogVxWorks64File, WriteWritesToTheOpenDescriptor)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_Write(file, "hello", 5);

    CALLED_FAKE(VxWorks64IoFake_Write, ONCE);
    LONGS_EQUAL(VxWorks64IoFake_Fd(), VxWorks64IoFake_LastWriteFd());
}

TEST(SolidSyslogVxWorks64File, WriteWritesTheBytesGiven)
{
    const char* data = "hello";
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_Write(file, data, 5);

    POINTERS_EQUAL(data, VxWorks64IoFake_LastWriteBuffer());
    LONGS_EQUAL(5, VxWorks64IoFake_LastWriteNBytes());
}

TEST(SolidSyslogVxWorks64File, WriteSyncsTheDescriptorAfterWriting)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_Write(file, "hello", 5);

    LONGS_EQUAL(VxWorks64IoFake_Fd(), VxWorks64IoFake_IoctlFd(0));
    LONGS_EQUAL(VX_FIOSYNC, VxWorks64IoFake_IoctlFunction(0));
}

TEST(SolidSyslogVxWorks64File, WriteCommitsTheFileSystemAfterSyncing)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_Write(file, "hello", 5);

    LONGS_EQUAL(VxWorks64IoFake_Fd(), VxWorks64IoFake_IoctlFd(1));
    LONGS_EQUAL(VX_FIOCOMMITFS, VxWorks64IoFake_IoctlFunction(1));
}

TEST(SolidSyslogVxWorks64File, WriteReturnsTrueWhenTheWholeCountIsWrittenAndSynced)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    CHECK_TRUE(SolidSyslogFile_Write(file, "hello", 5));
}

TEST(SolidSyslogVxWorks64File, WriteReturnsFalseOnAShortWrite)
{
    VxWorks64IoFake_WriteAccepts(3);
    SolidSyslogFile_Open(file, TEST_PATH);

    CHECK_FALSE(SolidSyslogFile_Write(file, "hello", 5));
}

TEST(SolidSyslogVxWorks64File, WriteDoesNotSyncAShortWrite)
{
    VxWorks64IoFake_WriteAccepts(3);
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_Write(file, "hello", 5);

    CALLED_FAKE(VxWorks64IoFake_Ioctl, NEVER);
}

TEST(SolidSyslogVxWorks64File, WriteReturnsFalseWhenWriteFails)
{
    VxWorks64IoFake_WriteAccepts(ERROR_RESULT);
    SolidSyslogFile_Open(file, TEST_PATH);

    CHECK_FALSE(SolidSyslogFile_Write(file, "hello", 5));
}

TEST(SolidSyslogVxWorks64File, WriteReturnsFalseWhenSyncFails)
{
    VxWorks64IoFake_FailIoctl(VX_FIOSYNC);
    SolidSyslogFile_Open(file, TEST_PATH);

    CHECK_FALSE(SolidSyslogFile_Write(file, "hello", 5));
}

TEST(SolidSyslogVxWorks64File, WriteDoesNotCommitWhenSyncFails)
{
    VxWorks64IoFake_FailIoctl(VX_FIOSYNC);
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_Write(file, "hello", 5);

    CALLED_FAKE(VxWorks64IoFake_Ioctl, ONCE);
}

TEST(SolidSyslogVxWorks64File, WriteReturnsTrueWhenTheFileSystemHasNoCommitToMake)
{
    VxWorks64IoFake_FailIoctl(VX_FIOCOMMITFS);
    SolidSyslogFile_Open(file, TEST_PATH);

    CHECK_TRUE(SolidSyslogFile_Write(file, "hello", 5));
}

TEST(SolidSyslogVxWorks64File, SeekToSeeksTheOpenDescriptor)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_SeekTo(file, 42);

    CALLED_FAKE(VxWorks64IoFake_Lseek, ONCE);
    LONGS_EQUAL(VxWorks64IoFake_Fd(), VxWorks64IoFake_LastLseekFd());
}

TEST(SolidSyslogVxWorks64File, SeekToSeeksToTheOffsetFromTheStart)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_SeekTo(file, 42);

    LONGS_EQUAL(42, VxWorks64IoFake_LastLseekOffset());
    LONGS_EQUAL(VX_SEEK_SET, VxWorks64IoFake_LastLseekWhence());
}

TEST(SolidSyslogVxWorks64File, SizeSeeksTheOpenDescriptorToItsEnd)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_Size(file);

    LONGS_EQUAL(VxWorks64IoFake_Fd(), VxWorks64IoFake_LastLseekFd());
    LONGS_EQUAL(0, VxWorks64IoFake_LastLseekOffset());
    LONGS_EQUAL(VX_SEEK_END, VxWorks64IoFake_LastLseekWhence());
}

TEST(SolidSyslogVxWorks64File, SizeIsWhereTheEndIs)
{
    VxWorks64IoFake_SetFileSize(1234);
    SolidSyslogFile_Open(file, TEST_PATH);

    LONGS_EQUAL(1234, SolidSyslogFile_Size(file));
}

TEST(SolidSyslogVxWorks64File, SizeIsZeroWhenTheSeekFails)
{
    VxWorks64IoFake_SetFileSize(1234);
    VxWorks64IoFake_FailLseeks();
    SolidSyslogFile_Open(file, TEST_PATH);

    LONGS_EQUAL(0, SolidSyslogFile_Size(file));
}

TEST(SolidSyslogVxWorks64File, TruncateTruncatesTheOpenDescriptorToNothing)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_Truncate(file);

    CALLED_FAKE(VxWorks64IoFake_Ioctl, ONCE);
    LONGS_EQUAL(VxWorks64IoFake_Fd(), VxWorks64IoFake_IoctlFd(0));
    LONGS_EQUAL(VX_FIOTRUNC, VxWorks64IoFake_IoctlFunction(0));
    LONGS_EQUAL(0, VxWorks64IoFake_IoctlArg(0));
}

TEST(SolidSyslogVxWorks64File, ExistsProbesThePathReadOnly)
{
    SolidSyslogFile_Exists(file, TEST_PATH);

    CALLED_FAKE(VxWorks64IoFake_Open, ONCE);
    STRCMP_EQUAL(TEST_PATH, VxWorks64IoFake_LastOpenName());
    LONGS_EQUAL(VX_O_RDONLY, VxWorks64IoFake_LastOpenFlags());
}

TEST(SolidSyslogVxWorks64File, ExistsIsTrueWhenThePathOpens)
{
    VxWorks64IoFake_PutFile(TEST_PATH);

    CHECK_TRUE(SolidSyslogFile_Exists(file, TEST_PATH));
}

TEST(SolidSyslogVxWorks64File, ExistsClosesTheProbe)
{
    VxWorks64IoFake_PutFile(TEST_PATH);

    SolidSyslogFile_Exists(file, TEST_PATH);

    CALLED_FAKE(VxWorks64IoFake_Close, ONCE);
    LONGS_EQUAL(VxWorks64IoFake_Fd(), VxWorks64IoFake_LastClosedFd());
}

TEST(SolidSyslogVxWorks64File, ExistsIsFalseWhenThePathDoesNotOpen)
{
    VxWorks64IoFake_FailOpens();

    CHECK_FALSE(SolidSyslogFile_Exists(file, TEST_PATH));
}

TEST(SolidSyslogVxWorks64File, ExistsLeavesTheFileClosed)
{
    SolidSyslogFile_Exists(file, TEST_PATH);

    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogVxWorks64File, DeleteRemovesThePath)
{
    SolidSyslogFile_Delete(file, TEST_PATH);

    CALLED_FAKE(VxWorks64IoFake_Remove, ONCE);
    STRCMP_EQUAL(TEST_PATH, VxWorks64IoFake_LastRemoveName());
}

TEST(SolidSyslogVxWorks64File, DeleteIsTrueWhenRemoveSucceeds)
{
    VxWorks64IoFake_PutFile(TEST_PATH);

    CHECK_TRUE(SolidSyslogFile_Delete(file, TEST_PATH));
}

TEST(SolidSyslogVxWorks64File, DeleteDoesNotProbeAPathItRemoved)
{
    VxWorks64IoFake_PutFile(TEST_PATH);

    SolidSyslogFile_Delete(file, TEST_PATH);

    CALLED_FAKE(VxWorks64IoFake_Open, NEVER);
}

TEST(SolidSyslogVxWorks64File, DeleteIsTrueWhenThePathWasAlreadyAbsent)
{
    VxWorks64IoFake_FailRemoves();
    VxWorks64IoFake_FailOpens();

    CHECK_TRUE(SolidSyslogFile_Delete(file, TEST_PATH));
}

TEST(SolidSyslogVxWorks64File, DeleteIsFalseWhenThePathRemains)
{
    VxWorks64IoFake_PutFile(TEST_PATH);
    VxWorks64IoFake_FailRemoves();

    CHECK_FALSE(SolidSyslogFile_Delete(file, TEST_PATH));
}

TEST(SolidSyslogVxWorks64File, DestroyOfAClosedFileClosesNothing)
{
    SolidSyslogVxWorks64File_Destroy(file);
    file = nullptr;

    CALLED_FAKE(VxWorks64IoFake_Close, NEVER);
}

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64FilePool)
{
    struct SolidSyslogFile* pooled[SOLIDSYSLOG_FILE_POOL_SIZE] = {};
    struct SolidSyslogFile* overflow                                 = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogVxWorks64File_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogVxWorks64File_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogVxWorks64File_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64FilePool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogVxWorks64File_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogVxWorks64FilePool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogVxWorks64File_Create();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogVxWorks64FileErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_FILE_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogVxWorks64FilePool, FallbackOpenReturnsFalse)
{
    FillPool();
    overflow = SolidSyslogVxWorks64File_Create();

    CHECK_FALSE(SolidSyslogFile_Open(overflow, TEST_PATH));
}

TEST(SolidSyslogVxWorks64FilePool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogVxWorks64File_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogVxWorks64FilePool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogVxWorks64File_Create();

    LONGS_EQUAL(SOLIDSYSLOG_FILE_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_FILE_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogVxWorks64FilePool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogVxWorks64File_Create();
    ConfigLockFake_Install();

    SolidSyslogVxWorks64File_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogVxWorks64FilePool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogFile stranger = {};

    SolidSyslogVxWorks64File_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogVxWorks64FilePool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogFile stranger = {};

    SolidSyslogVxWorks64File_Destroy(&stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogVxWorks64FileErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_FILE_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogVxWorks64FilePool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogVxWorks64File_Create();
    SolidSyslogVxWorks64File_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogVxWorks64File_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogVxWorks64FileErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_FILE_ERROR_UNKNOWN_DESTROY
    );
}
