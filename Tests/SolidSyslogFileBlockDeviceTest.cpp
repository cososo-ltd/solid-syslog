#include "ErrorHandlerFake.h"
#include "FileFake.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogFileBlockDeviceErrors.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

class TEST_SolidSyslogFileBlockDevice_AcquireSecondBlockPreservesFirstBlockContent_Test;
class TEST_SolidSyslogFileBlockDevice_AppendsAccumulateAtEnd_Test;
class TEST_SolidSyslogFileBlockDevice_OverlargeBlockIndexLeavesValidBlockUntouched_Test;
class TEST_SolidSyslogFileBlockDevice_ReadAtOffsetReturnsBytesFromOffset_Test;
class TEST_SolidSyslogFileBlockDevice_ReadFollowedByWriteOnSameBlockDoesNotOpenTwoHandles_Test;
class TEST_SolidSyslogFileBlockDevice_ReadReturnsAppendedBytes_Test;
class TEST_SolidSyslogFileBlockDevice_WriteAtMutatesByteInPlace_Test;

static const char* const TEST_PATH_PREFIX = "/tmp/blockdev_";

/* Reads `length` bytes from (blockIndex, offset) and asserts they equal `expected`.
 * Mirrors the CHECK_PRIVAL family in SolidSyslogTest.cpp — names the intent so
 * tests read as "block N at offset O contains 'foo'" rather than buf+memcmp boilerplate.
 * Macro (not function) so test failures report the caller's __FILE__/__LINE__. */
#define CHECK_BLOCK_CONTAINS(blockIndex, offset, expected, length)                                   \
    do                                                                                               \
    {                                                                                                \
        char checkBuf[(length) + 1] = {};                                                            \
        CHECK_TRUE(SolidSyslogBlockDevice_Read(device, (blockIndex), (offset), checkBuf, (length))); \
        MEMCMP_EQUAL((expected), checkBuf, (length));                                                \
    } while (0)

// clang-format off
TEST_GROUP(SolidSyslogFileBlockDevice)
{
    struct FileFakeStorage              fileStorage = {};
    struct SolidSyslogFile*             file        = nullptr;
    struct SolidSyslogBlockDevice*      device      = nullptr;

    void setup() override
    {
        file = FileFake_Create(&fileStorage);
        device = SolidSyslogFileBlockDevice_Create(file, TEST_PATH_PREFIX, 4096);
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

TEST(SolidSyslogFileBlockDevice, GetBlockSizeReturnsCreatedSize)
{
    LONGS_EQUAL(4096, SolidSyslogBlockDevice_GetBlockSize(device));
}

TEST(SolidSyslogFileBlockDevice, ZeroBlockSizeUsesDefault)
{
    SolidSyslogFileBlockDevice_Destroy(device);
    device = SolidSyslogFileBlockDevice_Create(file, TEST_PATH_PREFIX, 0);
    LONGS_EQUAL(SOLIDSYSLOG_FILE_DEFAULT_BLOCK_SIZE, SolidSyslogBlockDevice_GetBlockSize(device));
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
    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/blockdev_07.log"));
}

TEST(SolidSyslogFileBlockDevice, BlockFilenameWithTwoDigitIndex)
{
    SolidSyslogBlockDevice_Acquire(device, 42);
    CHECK_TRUE(SolidSyslogFile_Exists(file, "/tmp/blockdev_42.log"));
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

    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

/* S27.01 invariant: at most one SolidSyslogFile is open on any given block
 * file path at a time. The dual-handle precursor would Open the read handle
 * on the same path the write handle still held, tripping the FileFake
 * single-handle-per-path assertion at the second Open. This test mirrors the
 * BlockStore steady-state pattern (write record -> read record -> write
 * sent-flag in place) on a single block so a future regression to the
 * two-handle shape would fault here on the Read step. */
TEST(SolidSyslogFileBlockDevice, ReadFollowedByWriteOnSameBlockDoesNotOpenTwoHandles)
{
    SolidSyslogBlockDevice_Acquire(device, 0);
    SolidSyslogBlockDevice_Append(device, 0, "rec", 3);

    char buf[3] = {};
    CHECK_TRUE(SolidSyslogBlockDevice_Read(device, 0, 0, buf, 3));
    MEMCMP_EQUAL("rec", buf, 3);

    CHECK_TRUE(SolidSyslogBlockDevice_WriteAt(device, 0, 1, "X", 1));
    CHECK_BLOCK_CONTAINS(0, 0, "rXc", 3);
}

TEST(SolidSyslogFileBlockDevice, UseAfterDestroyIsCrashSafeViaNullBlockDeviceVtable)
{
    /* After Destroy the slot's abstract-base vtable is the shared NullBlockDevice's, so
     * vtable calls through the stale handle are safe no-ops rather than a NULL-fn-pointer
     * crash. NullBlockDevice methods return false / 0. */
    SolidSyslogFileBlockDevice_Destroy(device);

    CHECK_FALSE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 0));
    CHECK_FALSE(SolidSyslogBlockDevice_Dispose(device, 0));
    char buf[3] = {};
    CHECK_FALSE(SolidSyslogBlockDevice_Read(device, 0, 0, buf, 3));
    CHECK_FALSE(SolidSyslogBlockDevice_Append(device, 0, "x", 1));
    CHECK_FALSE(SolidSyslogBlockDevice_WriteAt(device, 0, 0, "x", 1));
    LONGS_EQUAL(0, SolidSyslogBlockDevice_Size(device, 0));

    device = nullptr; // teardown's nullptr guard skips the second Destroy
}

// Pool tests — prove SOLIDSYSLOG_FILE_BLOCK_DEVICE_POOL_SIZE caps live
// instances and overflow falls back to the class-private no-op BlockDevice.
// Generic pool mechanics (lock counts, per-probe locking, stale-handle warning)
// are covered by SolidSyslogPoolAllocatorTest.cpp.

// clang-format off
TEST_GROUP(SolidSyslogFileBlockDevicePool)
{
    struct FileFakeStorage              fileStorage = {};
    struct SolidSyslogFile*             file        = nullptr;
    struct SolidSyslogBlockDevice*      pooled[SOLIDSYSLOG_FILE_BLOCK_DEVICE_POOL_SIZE] = {};
    struct SolidSyslogBlockDevice*      overflow    = nullptr;

    void setup() override
    {
        file = FileFake_Create(&fileStorage);
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogFileBlockDevice_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogFileBlockDevice_Destroy(overflow);
        }
        FileFake_Destroy();
    }

    [[nodiscard]] struct SolidSyslogBlockDevice* MakeDevice() const
    {
        return SolidSyslogFileBlockDevice_Create(file, TEST_PATH_PREFIX, 4096);
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = MakeDevice();
        }
    }
};

// clang-format on

TEST(SolidSyslogFileBlockDevicePool, OverflowReportsPoolExhausted)
{
    FillPool();
    ErrorHandlerFake_Install(nullptr);

    overflow = MakeDevice();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_CRITICAL, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&FileBlockDeviceErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_POOL_EXHAUSTED, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(FILEBLOCKDEVICE_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogFileBlockDevicePool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = MakeDevice();

    CHECK_TEXT(overflow != nullptr, "Fallback handle was nullptr");
    for (auto* slot : pooled)
    {
        CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");
        CHECK_TEXT(overflow != slot, "Fallback handle collided with a pool slot");
    }
}
