#include "FileFake.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogFile.h"
#include "CppUTest/TestHarness.h"

class TEST_SolidSyslogFileBlockDevice_AcquireSecondBlockPreservesFirstBlockContent_Test;
class TEST_SolidSyslogFileBlockDevice_AppendsAccumulateAtEnd_Test;
class TEST_SolidSyslogFileBlockDevice_OverlargeBlockIndexLeavesValidBlockUntouched_Test;
class TEST_SolidSyslogFileBlockDevice_ReadAtOffsetReturnsBytesFromOffset_Test;
class TEST_SolidSyslogFileBlockDevice_ReadReturnsAppendedBytes_Test;
class TEST_SolidSyslogFileBlockDevice_WriteAtMutatesByteInPlace_Test;

static const char* const TEST_PATH_PREFIX = "/tmp/blockdev_";

/* Reads `length` bytes from (blockIndex, offset) and asserts they equal `expected`.
 * Mirrors the CHECK_PRIVAL family in SolidSyslogTest.cpp — names the intent so
 * tests read as "block N at offset O contains 'foo'" rather than buf+memcmp boilerplate.
 * Macro (not function) so test failures report the caller's __FILE__/__LINE__. */
// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while) -- macro preserves caller location in test failure output
#define CHECK_BLOCK_CONTAINS(blockIndex, offset, expected, length)                                   \
    do                                                                                               \
    {                                                                                                \
        char checkBuf[(length) + 1] = {};                                                            \
        CHECK_TRUE(SolidSyslogBlockDevice_Read(device, (blockIndex), (offset), checkBuf, (length))); \
        MEMCMP_EQUAL((expected), checkBuf, (length));                                                \
    } while (0)

// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

// clang-format off
TEST_GROUP(SolidSyslogFileBlockDevice)
{
    struct FileFakeStorage              readStorage  = {};
    struct FileFakeStorage              writeStorage = {};
    struct SolidSyslogFile*             readFile     = nullptr;
    struct SolidSyslogFile*             writeFile    = nullptr;
    SolidSyslogFileBlockDeviceStorage   deviceStorage = {};
    struct SolidSyslogBlockDevice*      device       = nullptr;

    void setup() override
    {
        readFile  = FileFake_Create(&readStorage);
        writeFile = FileFake_Create(&writeStorage);
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        device = SolidSyslogFileBlockDevice_Create(&deviceStorage, readFile, writeFile, TEST_PATH_PREFIX);
    }

    void teardown() override
    {
        if (device != nullptr)
        {
            SolidSyslogFileBlockDevice_Destroy(device);
        }
        FileFake_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogFileBlockDevice, CreateReturnsNonNull)
{
    CHECK_TRUE(device != nullptr);
}

TEST(SolidSyslogFileBlockDevice, ExistsReturnsFalseOnFreshSlate)
{
    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 0));
}

TEST(SolidSyslogFileBlockDevice, AcquireMakesBlockExist)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(SolidSyslogBlockDevice_Exists(device, 0));
}

TEST(SolidSyslogFileBlockDevice, AcquireFreshBlockHasZeroSize)
{
    SolidSyslogBlockDevice_Acquire(device, 0);
    LONGS_EQUAL(0, SolidSyslogBlockDevice_Size(device, 0));
}

TEST(SolidSyslogFileBlockDevice, ReacquireTruncatesExistingBlock)
{
    SolidSyslogBlockDevice_Acquire(device, 0);
    SolidSyslogBlockDevice_Append(device, 0, "abc", 3);

    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));

    LONGS_EQUAL(0, SolidSyslogBlockDevice_Size(device, 0));
}

TEST(SolidSyslogFileBlockDevice, AppendIncreasesBlockSize)
{
    SolidSyslogBlockDevice_Acquire(device, 0);
    CHECK_TRUE(SolidSyslogBlockDevice_Append(device, 0, "abc", 3));
    LONGS_EQUAL(3, SolidSyslogBlockDevice_Size(device, 0));
}

TEST(SolidSyslogFileBlockDevice, ReadReturnsAppendedBytes)
{
    SolidSyslogBlockDevice_Acquire(device, 0);
    SolidSyslogBlockDevice_Append(device, 0, "abc", 3);

    CHECK_BLOCK_CONTAINS(0, 0, "abc", 3);
}

TEST(SolidSyslogFileBlockDevice, AppendsAccumulateAtEnd)
{
    SolidSyslogBlockDevice_Acquire(device, 0);
    SolidSyslogBlockDevice_Append(device, 0, "abc", 3);
    SolidSyslogBlockDevice_Append(device, 0, "de", 2);

    LONGS_EQUAL(5, SolidSyslogBlockDevice_Size(device, 0));
    CHECK_BLOCK_CONTAINS(0, 0, "abcde", 5);
}

TEST(SolidSyslogFileBlockDevice, ReadAtOffsetReturnsBytesFromOffset)
{
    SolidSyslogBlockDevice_Acquire(device, 0);
    SolidSyslogBlockDevice_Append(device, 0, "abcde", 5);

    CHECK_BLOCK_CONTAINS(0, 2, "cd", 2);
}

TEST(SolidSyslogFileBlockDevice, WriteAtMutatesByteInPlace)
{
    SolidSyslogBlockDevice_Acquire(device, 0);
    SolidSyslogBlockDevice_Append(device, 0, "abcde", 5);

    CHECK_TRUE(SolidSyslogBlockDevice_WriteAt(device, 0, 1, "X", 1));

    CHECK_BLOCK_CONTAINS(0, 0, "aXcde", 5);
    LONGS_EQUAL(5, SolidSyslogBlockDevice_Size(device, 0));
}

TEST(SolidSyslogFileBlockDevice, AcquireSecondBlockPreservesFirstBlockContent)
{
    SolidSyslogBlockDevice_Acquire(device, 0);
    SolidSyslogBlockDevice_Append(device, 0, "first", 5);
    SolidSyslogBlockDevice_Acquire(device, 1);

    LONGS_EQUAL(0, SolidSyslogBlockDevice_Size(device, 1));
    LONGS_EQUAL(5, SolidSyslogBlockDevice_Size(device, 0));
    CHECK_BLOCK_CONTAINS(0, 0, "first", 5);
}

TEST(SolidSyslogFileBlockDevice, DisposeMakesBlockNotExist)
{
    SolidSyslogBlockDevice_Acquire(device, 0);
    CHECK_TRUE(SolidSyslogBlockDevice_Dispose(device, 0));
    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 0));
}

TEST(SolidSyslogFileBlockDevice, DisposeLeavesOtherBlocksUntouched)
{
    SolidSyslogBlockDevice_Acquire(device, 0);
    SolidSyslogBlockDevice_Acquire(device, 1);
    SolidSyslogBlockDevice_Append(device, 1, "keep", 4);

    SolidSyslogBlockDevice_Dispose(device, 0);

    CHECK_TRUE(SolidSyslogBlockDevice_Exists(device, 1));
    LONGS_EQUAL(4, SolidSyslogBlockDevice_Size(device, 1));
}

TEST(SolidSyslogFileBlockDevice, BlockFilenameZeroPadsSequenceToTwoDigits)
{
    SolidSyslogBlockDevice_Acquire(device, 7);
    CHECK_TRUE(SolidSyslogFile_Exists(writeFile, "/tmp/blockdev_07.log"));
}

TEST(SolidSyslogFileBlockDevice, BlockFilenameWithTwoDigitIndex)
{
    SolidSyslogBlockDevice_Acquire(device, 42);
    CHECK_TRUE(SolidSyslogFile_Exists(writeFile, "/tmp/blockdev_42.log"));
}

/* The on-disk sequence is two decimal digits: indices > 99 cannot be
 * represented uniquely. Without a guard, casting a wide blockIndex through
 * uint8_t (256 → 0) would alias to an existing block and silently overwrite
 * its content. */
TEST(SolidSyslogFileBlockDevice, AcquireRejectsOverlargeBlockIndex)
{
    CHECK_FALSE(SolidSyslogBlockDevice_Acquire(device, 100));
    CHECK_FALSE(SolidSyslogBlockDevice_Acquire(device, 256));
}

TEST(SolidSyslogFileBlockDevice, OverlargeBlockIndexLeavesValidBlockUntouched)
{
    SolidSyslogBlockDevice_Acquire(device, 0);
    SolidSyslogBlockDevice_Append(device, 0, "real", 4);

    SolidSyslogBlockDevice_Acquire(device, 256); /* would alias to block 0 if not guarded */

    LONGS_EQUAL(4, SolidSyslogBlockDevice_Size(device, 0));
    CHECK_BLOCK_CONTAINS(0, 0, "real", 4);
}

TEST(SolidSyslogFileBlockDevice, ExistsReturnsFalseForOverlargeBlockIndex)
{
    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 256));
}

TEST(SolidSyslogFileBlockDevice, ReadReturnsFalseForOverlargeBlockIndex)
{
    char buf[1] = {};
    CHECK_FALSE(SolidSyslogBlockDevice_Read(device, 256, 0, buf, 1));
}

TEST(SolidSyslogFileBlockDevice, AppendReturnsFalseForOverlargeBlockIndex)
{
    CHECK_FALSE(SolidSyslogBlockDevice_Append(device, 256, "x", 1));
}

TEST(SolidSyslogFileBlockDevice, WriteAtReturnsFalseForOverlargeBlockIndex)
{
    CHECK_FALSE(SolidSyslogBlockDevice_WriteAt(device, 256, 0, "x", 1));
}

TEST(SolidSyslogFileBlockDevice, DisposeReturnsFalseForOverlargeBlockIndex)
{
    CHECK_FALSE(SolidSyslogBlockDevice_Dispose(device, 256));
}

TEST(SolidSyslogFileBlockDevice, SizeReturnsZeroForOverlargeBlockIndex)
{
    LONGS_EQUAL(0, SolidSyslogBlockDevice_Size(device, 256));
}

TEST(SolidSyslogFileBlockDevice, DestroyClosesOpenFileHandles)
{
    SolidSyslogBlockDevice_Acquire(device, 0);
    SolidSyslogBlockDevice_Read(device, 0, 0, nullptr, 0);

    SolidSyslogFileBlockDevice_Destroy(device);
    device = nullptr;

    CHECK_FALSE(SolidSyslogFile_IsOpen(readFile));
    CHECK_FALSE(SolidSyslogFile_IsOpen(writeFile));
}
