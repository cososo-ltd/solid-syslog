#include "BlockDeviceFake.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogStore.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(BlockDeviceFake)
{
    struct SolidSyslogBlockDevice* device = nullptr;

    void setup() override
    {
        device = BlockDeviceFake_Create(64);
    }
};

// clang-format on

TEST(BlockDeviceFake, AFreshDeviceHasNoBlockPresent)
{
    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 0));
}

TEST(BlockDeviceFake, SizeOfAnAbsentBlockFails)
{
    size_t size = 99U;
    CHECK_FALSE(SolidSyslogBlockDevice_Size(device, 0, &size));
    LONGS_EQUAL(0, size);
}

TEST(BlockDeviceFake, ReadFromAnAbsentBlockFails)
{
    char buf[4] = {};
    CHECK_FALSE(SolidSyslogBlockDevice_Read(device, 0, 0, buf, sizeof(buf)));
}

TEST(BlockDeviceFake, AcquireMakesTheBlockPresent)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 3));
    CHECK_TRUE(SolidSyslogBlockDevice_Exists(device, 3));
}

TEST(BlockDeviceFake, AnAcquiredBlockIsEmpty)
{
    SolidSyslogBlockDevice_Acquire(device, 3);
    size_t size = 99U;
    CHECK_TRUE(SolidSyslogBlockDevice_Size(device, 3, &size));
    LONGS_EQUAL(0, size);
}

TEST(BlockDeviceFake, AppendGrowsTheBlock)
{
    SolidSyslogBlockDevice_Acquire(device, 3);
    CHECK_TRUE(SolidSyslogBlockDevice_Append(device, 3, "abc", 3));
    size_t size = 0U;
    SolidSyslogBlockDevice_Size(device, 3, &size);
    LONGS_EQUAL(3, size);
}

TEST(BlockDeviceFake, ReadReturnsAppendedBytesFromAnOffset)
{
    SolidSyslogBlockDevice_Acquire(device, 3);
    SolidSyslogBlockDevice_Append(device, 3, "abcdef", 6);
    char buf[3] = {};
    CHECK_TRUE(SolidSyslogBlockDevice_Read(device, 3, 2, buf, 3));
    MEMCMP_EQUAL("cde", buf, 3);
}

TEST(BlockDeviceFake, WriteAtOverwritesInPlaceWithoutGrowingTheBlock)
{
    SolidSyslogBlockDevice_Acquire(device, 3);
    SolidSyslogBlockDevice_Append(device, 3, "abcdef", 6);
    CHECK_TRUE(SolidSyslogBlockDevice_WriteAt(device, 3, 1, "XY", 2));
    char buf[6] = {};
    SolidSyslogBlockDevice_Read(device, 3, 0, buf, 6);
    MEMCMP_EQUAL("aXYdef", buf, 6);
    size_t size = 0U;
    SolidSyslogBlockDevice_Size(device, 3, &size);
    LONGS_EQUAL(6, size);
}

TEST(BlockDeviceFake, DisposeMakesTheBlockAbsent)
{
    SolidSyslogBlockDevice_Acquire(device, 3);
    CHECK_TRUE(SolidSyslogBlockDevice_Dispose(device, 3));
    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 3));
}

TEST(BlockDeviceFake, GetBlockSizeIsTheConfiguredSize)
{
    LONGS_EQUAL(64, SolidSyslogBlockDevice_GetBlockSize(device));
}

TEST(BlockDeviceFake, AcquireOfAPresentBlockEmptiesIt)
{
    SolidSyslogBlockDevice_Acquire(device, 3);
    SolidSyslogBlockDevice_Append(device, 3, "abc", 3);
    SolidSyslogBlockDevice_Acquire(device, 3);
    size_t size = 99U;
    SolidSyslogBlockDevice_Size(device, 3, &size);
    LONGS_EQUAL(0, size);
}

TEST(BlockDeviceFake, AppendPastTheBlockSizeFailsAndStoresNothing)
{
    char sixtyFive[65] = {};
    SolidSyslogBlockDevice_Acquire(device, 3);
    CHECK_FALSE(SolidSyslogBlockDevice_Append(device, 3, sixtyFive, sizeof(sixtyFive)));
    size_t size = 99U;
    SolidSyslogBlockDevice_Size(device, 3, &size);
    LONGS_EQUAL(0, size);
}

TEST(BlockDeviceFake, AppendToAnAbsentBlockFails)
{
    CHECK_FALSE(SolidSyslogBlockDevice_Append(device, 3, "abc", 3));
}

TEST(BlockDeviceFake, WriteAtToAnAbsentBlockFails)
{
    CHECK_FALSE(SolidSyslogBlockDevice_WriteAt(device, 3, 0, "a", 1));
}

TEST(BlockDeviceFake, ReadPastTheDataFails)
{
    SolidSyslogBlockDevice_Acquire(device, 3);
    SolidSyslogBlockDevice_Append(device, 3, "abc", 3);
    char buf[2] = {};
    CHECK_FALSE(SolidSyslogBlockDevice_Read(device, 3, 2, buf, 2));
}

TEST(BlockDeviceFake, WriteAtPastTheDataFails)
{
    SolidSyslogBlockDevice_Acquire(device, 3);
    SolidSyslogBlockDevice_Append(device, 3, "abc", 3);
    CHECK_FALSE(SolidSyslogBlockDevice_WriteAt(device, 3, 2, "XY", 2));
}

TEST(BlockDeviceFake, AnIndexOutsideTheBlockSpaceIsAbsentAndCannotBeAcquired)
{
    CHECK_FALSE(SolidSyslogBlockDevice_Acquire(device, 100));
    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 100));
}

TEST(BlockDeviceFake, EveryMediaOperationCountsOnce)
{
    char buf[1] = {};
    size_t size = 0U;
    SolidSyslogBlockDevice_Acquire(device, 3);
    SolidSyslogBlockDevice_Append(device, 3, "abc", 3);
    SolidSyslogBlockDevice_Read(device, 3, 0, buf, 1);
    SolidSyslogBlockDevice_WriteAt(device, 3, 0, "x", 1);
    SolidSyslogBlockDevice_Size(device, 3, &size);
    SolidSyslogBlockDevice_Exists(device, 3);
    SolidSyslogBlockDevice_Dispose(device, 3);
    SolidSyslogBlockDevice_GetBlockSize(device);
    LONGS_EQUAL(7, BlockDeviceFake_OperationCount());
}

TEST(BlockDeviceFake, TheWriteLogRecordsEveryAppendAndWriteAt)
{
    SolidSyslogBlockDevice_Acquire(device, 3);
    SolidSyslogBlockDevice_Append(device, 3, "abc", 3);
    SolidSyslogBlockDevice_WriteAt(device, 3, 1, "X", 1);
    LONGS_EQUAL(2, BlockDeviceFake_WriteCount());
    struct BlockDeviceFakeWrite first = BlockDeviceFake_Write(0);
    LONGS_EQUAL(3, first.Block);
    LONGS_EQUAL(0, first.Offset);
    LONGS_EQUAL(3, first.Count);
    CHECK_TRUE(first.Appended);
    struct BlockDeviceFakeWrite second = BlockDeviceFake_Write(1);
    LONGS_EQUAL(3, second.Block);
    LONGS_EQUAL(1, second.Offset);
    LONGS_EQUAL(1, second.Count);
    CHECK_FALSE(second.Appended);
}

TEST(BlockDeviceFake, AWriteTheDeviceRefusedIsNotLogged)
{
    SolidSyslogBlockDevice_WriteAt(device, 3, 0, "X", 1);
    LONGS_EQUAL(0, BlockDeviceFake_WriteCount());
}

TEST(BlockDeviceFake, CreateForgetsEverythingFromBefore)
{
    SolidSyslogBlockDevice_Acquire(device, 3);
    SolidSyslogBlockDevice_Append(device, 3, "abc", 3);
    device = BlockDeviceFake_Create(64);
    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 3));
    LONGS_EQUAL(1, BlockDeviceFake_OperationCount());
    LONGS_EQUAL(0, BlockDeviceFake_WriteCount());
}

TEST(BlockDeviceFake, TheBlockStoreRoundTripsARecordOverTheFake)
{
    device = BlockDeviceFake_Create(4096);
    struct SolidSyslogBlockStoreConfig config = {};
    config.BlockDevice = device;
    config.MaxBlocks = 2;
    struct SolidSyslogStore* store = SolidSyslogBlockStore_Create(&config);

    LONGS_EQUAL(SOLIDSYSLOG_STORE_WRITE_KEPT, SolidSyslogStore_Write(store, "hello", 5));
    char buf[8] = {};
    size_t bytesRead = 0U;
    CHECK_TRUE(SolidSyslogStore_ReadNextUnsent(store, buf, sizeof(buf), &bytesRead));
    LONGS_EQUAL(5, bytesRead);
    MEMCMP_EQUAL("hello", buf, 5);
    SolidSyslogStore_MarkSent(store);
    CHECK_FALSE(SolidSyslogStore_HasUnsent(store));

    SolidSyslogBlockStore_Destroy(store);
}

// clang-format off
TEST_GROUP(BlockDeviceFakeFaults)
{
    struct SolidSyslogBlockDevice* device = nullptr;
    char buf[8] = {};

    void setup() override
    {
        device = BlockDeviceFake_Create(64);
        SolidSyslogBlockDevice_Acquire(device, 3);
        SolidSyslogBlockDevice_Append(device, 3, "abcdef", 6);
    }

    bool ReadBlock3(size_t offset, size_t count)
    {
        return SolidSyslogBlockDevice_Read(device, 3, offset, buf, count);
    }
};

// clang-format on

TEST(BlockDeviceFakeFaults, FlipBitChangesTheByteOnTheMedia)
{
    BlockDeviceFake_FlipBit(3, 1, 0); /* 'b' is 0x62; bit 0 set gives 'c' */
    CHECK_TRUE(ReadBlock3(0, 6));
    MEMCMP_EQUAL("accdef", buf, 6);
}

TEST(BlockDeviceFakeFaults, AStuckByteReadsAsItsValueEvenAfterAWriteOverIt)
{
    BlockDeviceFake_StickByte(3, 2, 0xFF);
    SolidSyslogBlockDevice_WriteAt(device, 3, 2, "Z", 1);
    CHECK_TRUE(ReadBlock3(1, 3));
    BYTES_EQUAL('b', buf[0]);
    BYTES_EQUAL(0xFF, buf[1]);
    BYTES_EQUAL('d', buf[2]);
}

TEST(BlockDeviceFakeFaults, AReadTouchingAFailedOffsetFailsAndOthersSucceed)
{
    BlockDeviceFake_FailReadsTouching(3, 4);
    CHECK_FALSE(ReadBlock3(3, 2));
    CHECK_FALSE(ReadBlock3(4, 1));
    CHECK_TRUE(ReadBlock3(0, 4));
    CHECK_TRUE(ReadBlock3(5, 1));
}

TEST(BlockDeviceFakeFaults, TruncateToShortensTheReportedSizeAndBoundsReads)
{
    BlockDeviceFake_TruncateTo(3, 4);
    size_t size = 0U;
    CHECK_TRUE(SolidSyslogBlockDevice_Size(device, 3, &size));
    LONGS_EQUAL(4, size);
    CHECK_TRUE(ReadBlock3(0, 4));
    CHECK_FALSE(ReadBlock3(3, 2));
}

TEST(BlockDeviceFakeFaults, RemoveBlockMakesItAbsent)
{
    BlockDeviceFake_RemoveBlock(3);
    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 3));
    CHECK_FALSE(ReadBlock3(0, 1));
}

TEST(BlockDeviceFakeFaults, ABlockThatFailsEveryOperationStillReportsPresent)
{
    BlockDeviceFake_FailEveryOperationOn(3);
    size_t size = 0U;
    CHECK_TRUE(SolidSyslogBlockDevice_Exists(device, 3));
    CHECK_FALSE(SolidSyslogBlockDevice_Size(device, 3, &size));
    CHECK_FALSE(ReadBlock3(0, 1));
    CHECK_FALSE(SolidSyslogBlockDevice_Append(device, 3, "x", 1));
    CHECK_FALSE(SolidSyslogBlockDevice_WriteAt(device, 3, 0, "x", 1));
    CHECK_FALSE(SolidSyslogBlockDevice_Dispose(device, 3));
    CHECK_FALSE(SolidSyslogBlockDevice_Acquire(device, 3));
}

TEST(BlockDeviceFakeFaults, ABlockIsRemovedOnceTheOperationCountIsReached)
{
    BlockDeviceFake_RemoveBlockAfterOperation(3, 3); /* setup made two */
    CHECK_TRUE(SolidSyslogBlockDevice_Exists(device, 3));
    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 3));
}

TEST(BlockDeviceFakeFaults, ATornAppendLandsOnlyTheFirstBytesAndReportsSuccess)
{
    BlockDeviceFake_TearNextAppend(2);
    CHECK_TRUE(SolidSyslogBlockDevice_Append(device, 3, "XYZ", 3));
    size_t size = 0U;
    SolidSyslogBlockDevice_Size(device, 3, &size);
    LONGS_EQUAL(8, size);
    CHECK_TRUE(ReadBlock3(6, 2));
    MEMCMP_EQUAL("XY", buf, 2);
    CHECK_TRUE(SolidSyslogBlockDevice_Append(device, 3, "Q", 1));
    SolidSyslogBlockDevice_Size(device, 3, &size);
    LONGS_EQUAL(9, size);
}

TEST(BlockDeviceFakeFaults, WhenAbsentBlocksAreCreatedOnTouchSizeIsZeroAndTheBlockExistsAfter)
{
    BlockDeviceFake_SetAbsentBlockBehaviour(BLOCKDEVICEFAKE_ABSENT_IS_CREATED_ON_TOUCH);
    size_t size = 99U;
    CHECK_TRUE(SolidSyslogBlockDevice_Size(device, 7, &size));
    LONGS_EQUAL(0, size);
    CHECK_TRUE(SolidSyslogBlockDevice_Exists(device, 7));
}

TEST(BlockDeviceFakeFaults, WhenAbsentBlocksAreGhostsOperationsUseTheOldContentButExistsSaysAbsent)
{
    BlockDeviceFake_RemoveBlock(3);
    BlockDeviceFake_SetAbsentBlockBehaviour(BLOCKDEVICEFAKE_ABSENT_IS_GHOST);
    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 3));
    CHECK_TRUE(ReadBlock3(0, 6));
    MEMCMP_EQUAL("abcdef", buf, 6);
    CHECK_TRUE(SolidSyslogBlockDevice_Append(device, 3, "g", 1));
}
