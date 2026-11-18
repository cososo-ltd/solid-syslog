#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ErrorHandlerFake.h"
#include "NorFlashFake.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNorFlashBlockDevice.h"
#include "SolidSyslogNorFlashBlockDeviceErrors.h"
#include "SolidSyslogNorFlashCategories.h"
#include "SolidSyslogNullBlockDevice.h"
#include "SolidSyslogNullNorFlash.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

enum
{
    FLASH_BLOCK_SIZE = 256,
    FLASH_BLOCK_COUNT = 8,
    /* The device's run of the flash: its block 0 is the flash's block 2. */
    FIRST_BLOCK = 2,
    BLOCK_COUNT = 4,
    STATE_SIZE = SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_STATE_SIZE,
    DATA_BYTES = FLASH_BLOCK_SIZE - STATE_SIZE
};

/* Reads `length` bytes from (blockIndex, offset) and asserts they equal
 * `expected`. A macro, so a failure is reported at the test's own line. */
#define CHECK_BLOCK_CONTAINS(blockIndex, offset, expected, length)                                   \
    {                                                                                                \
        char checkBuf[(length) + 1] = {};                                                            \
        CHECK_TRUE(SolidSyslogBlockDevice_Read(device, (blockIndex), (offset), checkBuf, (length))); \
        MEMCMP_EQUAL((expected), checkBuf, (length));                                                \
    }

#define CHECK_BLOCK_SIZE(blockIndex, expected)                                     \
    {                                                                              \
        size_t checkSize = 0U;                                                     \
        CHECK_TRUE(SolidSyslogBlockDevice_Size(device, (blockIndex), &checkSize)); \
        LONGS_EQUAL((expected), checkSize);                                        \
    }

/* The four bytes of state at the head of the device's block `blockIndex`. */
#define CHECK_BLOCK_STATE(blockIndex, b0, b1, b2, b3)                           \
    {                                                                           \
        BYTES_EQUAL((b0), NorFlashFake_Peek(FIRST_BLOCK + (blockIndex), 0));    \
        BYTES_EQUAL((b1), NorFlashFake_Peek(FIRST_BLOCK + (blockIndex), 1));    \
        BYTES_EQUAL((b2), NorFlashFake_Peek(FIRST_BLOCK + (blockIndex), 2));    \
        BYTES_EQUAL((b3), NorFlashFake_Peek(FIRST_BLOCK + (blockIndex), 3));    \
    }

#define CHECK_BAD_CONFIG_REPORTED(detail)              \
    CHECK_ERROR_REPORTED_ONCE(                         \
        SOLIDSYSLOG_SEVERITY_CRITICAL,                 \
        &SolidSyslogNorFlashBlockDeviceErrorSource,    \
        SOLIDSYSLOG_CAT_BAD_CONFIG,                    \
        (detail)                                       \
    )

#define CHECK_VERIFY_FAILURE_REPORTED()                       \
    CHECK_ERROR_REPORTED_ONCE(                                \
        SOLIDSYSLOG_SEVERITY_ERROR,                           \
        &SolidSyslogNorFlashBlockDeviceErrorSource,           \
        SOLIDSYSLOG_CAT_NOR_FLASH_IO_FAILED,                  \
        SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_VERIFY_FAILED \
    )

// clang-format off
TEST_BASE(NorFlashBlockDeviceTestBase)
{
    struct SolidSyslogNorFlash*    flash  = nullptr;
    struct SolidSyslogBlockDevice* device = nullptr;

    void SetupFlash(size_t blockSize = FLASH_BLOCK_SIZE)
    {
        flash = NorFlashFake_Create(blockSize, FLASH_BLOCK_COUNT);
    }

    void DestroyDevice()
    {
        if (device != nullptr)
        {
            SolidSyslogNorFlashBlockDevice_Destroy(device);
            device = nullptr;
        }
    }

    void CreateDevice(size_t firstBlock = FIRST_BLOCK, size_t blockCount = BLOCK_COUNT, size_t blockSize = 0)
    {
        struct SolidSyslogNorFlashBlockDeviceConfig config = {};
        config.Flash      = flash;
        config.FirstBlock = firstBlock;
        config.BlockCount = blockCount;
        config.BlockSize  = blockSize;
        device = SolidSyslogNorFlashBlockDevice_Create(&config);
    }

    /* A device created afresh over what is on the flash, as a restart does. */
    void Restart()
    {
        DestroyDevice();
        CreateDevice();
    }

    [[nodiscard]] bool Append(size_t blockIndex, const char* text) const
    {
        return SolidSyslogBlockDevice_Append(device, blockIndex, text, strlen(text));
    }
};

TEST_GROUP_BASE(SolidSyslogNorFlashBlockDeviceConfig, NorFlashBlockDeviceTestBase)
{
    void setup() override
    {
        SetupFlash();
        ErrorHandlerFake_Install(nullptr);
    }

    void teardown() override
    {
        if (device != SolidSyslogNullBlockDevice_Get())
        {
            DestroyDevice();
        }
    }
};

TEST_GROUP_BASE(SolidSyslogNorFlashBlockDevice, NorFlashBlockDeviceTestBase)
{
    void setup() override
    {
        SetupFlash();
        CreateDevice();
    }

    void teardown() override
    {
        DestroyDevice();
    }
};

// clang-format on

TEST(SolidSyslogNorFlashBlockDeviceConfig, ANullConfigIsReportedAndGivesTheNullDevice)
{
    device = SolidSyslogNorFlashBlockDevice_Create(nullptr);

    POINTERS_EQUAL(SolidSyslogNullBlockDevice_Get(), device);
    CHECK_BAD_CONFIG_REPORTED(SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_NULL_CONFIG);
}

TEST(SolidSyslogNorFlashBlockDeviceConfig, ANullFlashIsReportedAndGivesTheNullDevice)
{
    flash = nullptr;

    CreateDevice();

    POINTERS_EQUAL(SolidSyslogNullBlockDevice_Get(), device);
    CHECK_BAD_CONFIG_REPORTED(SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_NULL_FLASH);
}

TEST(SolidSyslogNorFlashBlockDeviceConfig, TheNullFlashGivesTheNullDeviceAndReportsNothing)
{
    flash = SolidSyslogNullNorFlash_Get();

    CreateDevice();

    POINTERS_EQUAL(SolidSyslogNullBlockDevice_Get(), device);
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogNorFlashBlockDeviceConfig, ARunThatEndsPastTheFlashIsRefused)
{
    CreateDevice(FLASH_BLOCK_COUNT - 1, 2);

    POINTERS_EQUAL(SolidSyslogNullBlockDevice_Get(), device);
    CHECK_BAD_CONFIG_REPORTED(SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_BLOCKS_OUTSIDE_FLASH);
}

TEST(SolidSyslogNorFlashBlockDeviceConfig, ARunOfMoreBlocksThanTheFlashHasIsRefused)
{
    CreateDevice(0, FLASH_BLOCK_COUNT + 1);

    POINTERS_EQUAL(SolidSyslogNullBlockDevice_Get(), device);
    CHECK_BAD_CONFIG_REPORTED(SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_BLOCKS_OUTSIDE_FLASH);
}

TEST(SolidSyslogNorFlashBlockDeviceConfig, ARunThatEndsWithTheFlashIsTaken)
{
    CreateDevice(FLASH_BLOCK_COUNT - 2, 2);

    CHECK_TRUE(device != SolidSyslogNullBlockDevice_Get());
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogNorFlashBlockDeviceConfig, ABlockSizeMoreThanAFlashBlockHoldsIsRefused)
{
    CreateDevice(FIRST_BLOCK, BLOCK_COUNT, DATA_BYTES + 1);

    POINTERS_EQUAL(SolidSyslogNullBlockDevice_Get(), device);
    CHECK_BAD_CONFIG_REPORTED(SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_BLOCK_SIZE_UNAVAILABLE);
}

TEST(SolidSyslogNorFlashBlockDeviceConfig, AFlashBlockThatHoldsOnlyTheStateIsRefused)
{
    SetupFlash(STATE_SIZE);

    CreateDevice();

    POINTERS_EQUAL(SolidSyslogNullBlockDevice_Get(), device);
    CHECK_BAD_CONFIG_REPORTED(SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_BLOCK_SIZE_UNAVAILABLE);
}

TEST(SolidSyslogNorFlashBlockDeviceConfig, ABlockSizeOfZeroTakesAllAFlashBlockHoldsAfterTheState)
{
    CreateDevice(FIRST_BLOCK, BLOCK_COUNT, 0);

    LONGS_EQUAL(DATA_BYTES, SolidSyslogBlockDevice_GetBlockSize(device));
}

TEST(SolidSyslogNorFlashBlockDeviceConfig, ABlockSizeAFlashBlockHoldsIsTheBlockSize)
{
    CreateDevice(FIRST_BLOCK, BLOCK_COUNT, DATA_BYTES);

    LONGS_EQUAL(DATA_BYTES, SolidSyslogBlockDevice_GetBlockSize(device));
}

TEST(SolidSyslogNorFlashBlockDeviceConfig, ASmallerBlockSizeIsTheBlockSize)
{
    CreateDevice(FIRST_BLOCK, BLOCK_COUNT, 100);

    LONGS_EQUAL(100, SolidSyslogBlockDevice_GetBlockSize(device));
}

TEST(SolidSyslogNorFlashBlockDeviceConfig, TheBlockCountIsTheRunGiven)
{
    CreateDevice(FIRST_BLOCK, BLOCK_COUNT);

    LONGS_EQUAL(BLOCK_COUNT, SolidSyslogBlockDevice_GetBlockCount(device));
}

TEST(SolidSyslogNorFlashBlockDeviceConfig, CreateTouchesNothingOnTheFlash)
{
    CreateDevice();

    LONGS_EQUAL(0, NorFlashFake_ReadCount());
    LONGS_EQUAL(0, NorFlashFake_WriteOperationCount());
}

TEST(SolidSyslogNorFlashBlockDevice, TheErrorSourceNamesItsDetailsThroughNorFlashBlockDeviceErrors)
{
    CHECK(SolidSyslogNorFlashBlockDeviceErrors_Name == SolidSyslogNorFlashBlockDeviceErrorSource.DetailName);
}

TEST(SolidSyslogNorFlashBlockDevice, ABlankBlockDoesNotExist)
{
    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 0));
}

TEST(SolidSyslogNorFlashBlockDevice, AnAcquiredBlockExists)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));

    CHECK_TRUE(SolidSyslogBlockDevice_Exists(device, 0));
}

TEST(SolidSyslogNorFlashBlockDevice, AnAcquiredBlockIsEmpty)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));

    CHECK_BLOCK_SIZE(0, 0);
}

TEST(SolidSyslogNorFlashBlockDevice, ABlockKeepsItsStateInItsOwnBlockOfTheRun)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 1));

    CHECK_BLOCK_STATE(1, 0x5A, 0xA5, 0xFF, 0xFF);
    CHECK_BLOCK_STATE(0, 0xFF, 0xFF, 0xFF, 0xFF);
    BYTES_EQUAL(0xFF, NorFlashFake_Peek(1, 0)); /* the flash's own block 1, outside the run */
}

TEST(SolidSyslogNorFlashBlockDevice, ABlockPastTheRunDoesNotExist)
{
    NorFlashFake_Poke(FIRST_BLOCK + BLOCK_COUNT, 0, 0x5A);
    NorFlashFake_Poke(FIRST_BLOCK + BLOCK_COUNT, 1, 0xA5);

    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, BLOCK_COUNT));
}

TEST(SolidSyslogNorFlashBlockDevice, ABlockWhoseStateCannotBeReadDoesNotExist)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    NorFlashFake_FailNextRead();

    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 0));
}

TEST(SolidSyslogNorFlashBlockDevice, AcquiringABlankBlockErasesIt)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));

    LONGS_EQUAL(1, NorFlashFake_EraseCountOf(FIRST_BLOCK));
}

TEST(SolidSyslogNorFlashBlockDevice, AcquiringABlockPastTheRunFails)
{
    CHECK_FALSE(SolidSyslogBlockDevice_Acquire(device, BLOCK_COUNT));

    LONGS_EQUAL(0, NorFlashFake_WriteOperationCount());
}

TEST(SolidSyslogNorFlashBlockDevice, AcquireFailsWhenTheEraseFails)
{
    NorFlashFake_FailNextErase();

    CHECK_FALSE(SolidSyslogBlockDevice_Acquire(device, 0));

    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 0));
}

TEST(SolidSyslogNorFlashBlockDevice, AcquireFailsWhenTheStateCannotBeProgrammed)
{
    NorFlashFake_FailNextProgram();

    CHECK_FALSE(SolidSyslogBlockDevice_Acquire(device, 0));
}

TEST(SolidSyslogNorFlashBlockDevice, AcquireFailsAndReportsWhenTheStateDoesNotTake)
{
    ErrorHandlerFake_Install(nullptr);
    NorFlashFake_IgnoreNextProgram();

    CHECK_FALSE(SolidSyslogBlockDevice_Acquire(device, 0));

    CHECK_VERIFY_FAILURE_REPORTED();
}

TEST(SolidSyslogNorFlashBlockDevice, AcquiringABlockInUseEmptiesIt)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "hello"));

    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));

    CHECK_BLOCK_SIZE(0, 0);
    LONGS_EQUAL(2, NorFlashFake_EraseCountOf(FIRST_BLOCK));
}

TEST(SolidSyslogNorFlashBlockDevice, AppendedBytesReadBack)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));

    CHECK_TRUE(Append(0, "hello"));

    CHECK_BLOCK_CONTAINS(0, 0, "hello", 5);
}

TEST(SolidSyslogNorFlashBlockDevice, AppendsAccumulateAtTheEnd)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "hello"));

    CHECK_TRUE(Append(0, "world"));

    CHECK_BLOCK_CONTAINS(0, 0, "helloworld", 10);
    CHECK_BLOCK_SIZE(0, 10);
}

TEST(SolidSyslogNorFlashBlockDevice, TheDataFollowsTheStateInTheFlashBlock)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));

    CHECK_TRUE(Append(0, "h"));

    BYTES_EQUAL('h', NorFlashFake_Peek(FIRST_BLOCK, STATE_SIZE));
}

TEST(SolidSyslogNorFlashBlockDevice, AnAppendToTheBlockLastAppendedToReadsOnlyToCheckWhatItProgrammed)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "hello"));
    NorFlashFake_ResetCounts();

    CHECK_TRUE(Append(0, "world"));

    LONGS_EQUAL(1, NorFlashFake_ReadCount());
    LONGS_EQUAL(1, NorFlashFake_ProgramCount());
}

TEST(SolidSyslogNorFlashBlockDevice, AnAppendLongerThanOneCheckIsCheckedThroughout)
{
    char longText[150];
    memset(longText, 'x', sizeof(longText));
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));

    CHECK_TRUE(SolidSyslogBlockDevice_Append(device, 0, longText, sizeof(longText)));

    CHECK_BLOCK_CONTAINS(0, 0, longText, sizeof(longText));
}

TEST(SolidSyslogNorFlashBlockDevice, AMismatchPastTheFirstCheckIsFound)
{
    char longText[150];
    memset(longText, 'x', sizeof(longText));
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    NorFlashFake_Poke(FIRST_BLOCK, STATE_SIZE + 140, 0x00); /* a cell that is not erased */
    ErrorHandlerFake_Install(nullptr);

    CHECK_FALSE(SolidSyslogBlockDevice_Append(device, 0, longText, sizeof(longText)));

    CHECK_VERIFY_FAILURE_REPORTED();
}

TEST(SolidSyslogNorFlashBlockDevice, AnAppendThatFillsTheBlockIsTaken)
{
    char whole[DATA_BYTES];
    memset(whole, 'x', sizeof(whole));
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));

    CHECK_TRUE(SolidSyslogBlockDevice_Append(device, 0, whole, sizeof(whole)));

    CHECK_BLOCK_SIZE(0, DATA_BYTES);
}

TEST(SolidSyslogNorFlashBlockDevice, AnAppendThatDoesNotFitFailsAndProgramsNothing)
{
    char whole[DATA_BYTES];
    memset(whole, 'x', sizeof(whole));
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "h"));
    NorFlashFake_ResetCounts();

    CHECK_FALSE(SolidSyslogBlockDevice_Append(device, 0, whole, sizeof(whole)));

    LONGS_EQUAL(0, NorFlashFake_ProgramCount());
}

TEST(SolidSyslogNorFlashBlockDevice, ASmallerBlockSizeIsAllAnAppendMayFill)
{
    DestroyDevice();
    CreateDevice(FIRST_BLOCK, BLOCK_COUNT, 8);
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "12345678"));

    CHECK_FALSE(Append(0, "9"));
}

TEST(SolidSyslogNorFlashBlockDevice, AnAppendPastTheRunFails)
{
    CHECK_FALSE(Append(BLOCK_COUNT, "hello"));

    LONGS_EQUAL(0, NorFlashFake_WriteOperationCount());
}

TEST(SolidSyslogNorFlashBlockDevice, AnAppendFailsWhenTheProgramFails)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    NorFlashFake_FailNextProgram();

    CHECK_FALSE(Append(0, "hello"));
}

TEST(SolidSyslogNorFlashBlockDevice, AnAppendThatDoesNotTakeFailsAndIsReported)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    ErrorHandlerFake_Install(nullptr);
    NorFlashFake_IgnoreNextProgram();

    CHECK_FALSE(Append(0, "hello"));

    CHECK_VERIFY_FAILURE_REPORTED();
}

TEST(SolidSyslogNorFlashBlockDevice, AnAppendWhoseReadBackFailsFailsAndLeavesTheReportToTheFlash)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    ErrorHandlerFake_Install(nullptr);
    NorFlashFake_FailNextRead();

    CHECK_FALSE(Append(0, "hello"));

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogNorFlashBlockDevice, AfterAnAppendThatFailedTheNextLooksForTheEndAgain)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "hello"));
    NorFlashFake_CutPowerAt(1, NORFLASHFAKE_CUT_TORN); /* "wo" lands, "rld" does not */
    CHECK_FALSE(Append(0, "world"));
    NorFlashFake_PowerOn();

    CHECK_TRUE(Append(0, "again"));

    CHECK_BLOCK_CONTAINS(0, 0, "hellowoagain", 12);
}

TEST(SolidSyslogNorFlashBlockDevice, AfterARestartTheSizeIsWhereTheDataEnds)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "hello"));

    Restart();

    CHECK_BLOCK_SIZE(0, 5);
}

TEST(SolidSyslogNorFlashBlockDevice, AfterARestartAnEmptyBlockInUseHasNoSize)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));

    Restart();

    CHECK_BLOCK_SIZE(0, 0);
}

TEST(SolidSyslogNorFlashBlockDevice, AfterARestartTheSizeOfAFullBlockIsTheBlockSize)
{
    char whole[DATA_BYTES];
    memset(whole, 'x', sizeof(whole));
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(SolidSyslogBlockDevice_Append(device, 0, whole, sizeof(whole)));

    Restart();

    CHECK_BLOCK_SIZE(0, DATA_BYTES);
}

TEST(SolidSyslogNorFlashBlockDevice, ErasedBytesInsideTheDataAreNotItsEnd)
{
    const uint8_t record[] = {'a', 0xFF, 0xFF, 0xFF, 'b'};
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(SolidSyslogBlockDevice_Append(device, 0, record, sizeof(record)));

    Restart();

    CHECK_BLOCK_SIZE(0, sizeof(record));
}

TEST(SolidSyslogNorFlashBlockDevice, AfterARestartAnAppendLandsAfterTheData)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "hello"));
    Restart();

    CHECK_TRUE(Append(0, "world"));

    CHECK_BLOCK_CONTAINS(0, 0, "helloworld", 10);
}

TEST(SolidSyslogNorFlashBlockDevice, AnAppendFailsWhenTheEndOfTheDataCannotBeRead)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "hello"));
    Restart();
    size_t writesBefore = NorFlashFake_WriteOperationCount();
    NorFlashFake_FailNextRead();

    CHECK_FALSE(Append(0, "world"));

    LONGS_EQUAL(writesBefore, NorFlashFake_WriteOperationCount());
}

TEST(SolidSyslogNorFlashBlockDevice, AppendingToOneBlockDoesNotMoveTheEndOfAnother)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "hello"));
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 1));
    CHECK_TRUE(Append(1, "other"));

    CHECK_TRUE(Append(0, "world"));

    CHECK_BLOCK_CONTAINS(0, 0, "helloworld", 10);
    CHECK_BLOCK_CONTAINS(1, 0, "other", 5);
}

TEST(SolidSyslogNorFlashBlockDevice, TheSizeOfABlockNotInUseIsNotKnown)
{
    size_t size = 99U;

    CHECK_FALSE(SolidSyslogBlockDevice_Size(device, 0, &size));

    LONGS_EQUAL(0, size);
}

TEST(SolidSyslogNorFlashBlockDevice, TheSizeOfABlockPastTheRunIsNotKnown)
{
    size_t size = 99U;

    CHECK_FALSE(SolidSyslogBlockDevice_Size(device, BLOCK_COUNT, &size));
}

TEST(SolidSyslogNorFlashBlockDevice, TheSizeIsNotKnownWhenTheDataCannotBeRead)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "hello"));
    Restart();
    NorFlashFake_FailReadsTouching(FIRST_BLOCK, STATE_SIZE + 100);
    size_t size = 99U;

    CHECK_FALSE(SolidSyslogBlockDevice_Size(device, 0, &size));
}

TEST(SolidSyslogNorFlashBlockDevice, ReadAtAnOffsetReturnsTheBytesFromThere)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "helloworld"));

    CHECK_BLOCK_CONTAINS(0, 5, "world", 5);
}

TEST(SolidSyslogNorFlashBlockDevice, AReadToTheEndOfTheBlockIsTaken)
{
    char buf[4] = {};

    CHECK_TRUE(SolidSyslogBlockDevice_Read(device, 0, DATA_BYTES - sizeof(buf), buf, sizeof(buf)));
}

TEST(SolidSyslogNorFlashBlockDevice, AReadThatRunsPastTheBlockFails)
{
    char buf[4] = {};

    CHECK_FALSE(SolidSyslogBlockDevice_Read(device, 0, DATA_BYTES - sizeof(buf) + 1, buf, sizeof(buf)));

    LONGS_EQUAL(0, NorFlashFake_ReadCount());
}

TEST(SolidSyslogNorFlashBlockDevice, AReadOfMoreThanABlockFails)
{
    static char buf[DATA_BYTES + 1] = {};

    CHECK_FALSE(SolidSyslogBlockDevice_Read(device, 0, 0, buf, sizeof(buf)));
}

TEST(SolidSyslogNorFlashBlockDevice, AReadPastTheRunFails)
{
    char buf[4] = {};

    CHECK_FALSE(SolidSyslogBlockDevice_Read(device, BLOCK_COUNT, 0, buf, sizeof(buf)));
}

TEST(SolidSyslogNorFlashBlockDevice, AReadFailsWhenTheFlashRefusesIt)
{
    char buf[4] = {};
    NorFlashFake_FailNextRead();

    CHECK_FALSE(SolidSyslogBlockDevice_Read(device, 0, 0, buf, sizeof(buf)));
}

TEST(SolidSyslogNorFlashBlockDevice, WriteAtClearsBitsInPlace)
{
    const uint8_t unsent = 0xF0;
    const uint8_t sent = 0x00;
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "hello"));
    CHECK_TRUE(SolidSyslogBlockDevice_Append(device, 0, &unsent, 1));

    CHECK_TRUE(SolidSyslogBlockDevice_WriteAt(device, 0, 5, &sent, 1));

    CHECK_BLOCK_CONTAINS(0, 0, "hello\0", 6);
}

TEST(SolidSyslogNorFlashBlockDevice, AWriteAtThatWouldSetABitFailsAndIsReported)
{
    const uint8_t sent = 0x00;
    const uint8_t unsent = 0xF0;
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(SolidSyslogBlockDevice_Append(device, 0, &sent, 1));
    ErrorHandlerFake_Install(nullptr);

    CHECK_FALSE(SolidSyslogBlockDevice_WriteAt(device, 0, 0, &unsent, 1));

    CHECK_VERIFY_FAILURE_REPORTED();
}

TEST(SolidSyslogNorFlashBlockDevice, AWriteAtThatRunsPastTheBlockFails)
{
    const uint8_t bytes[2] = {0x00, 0x00};

    CHECK_FALSE(SolidSyslogBlockDevice_WriteAt(device, 0, DATA_BYTES - 1, bytes, sizeof(bytes)));

    LONGS_EQUAL(0, NorFlashFake_WriteOperationCount());
}

TEST(SolidSyslogNorFlashBlockDevice, AWriteAtPastTheRunFails)
{
    const uint8_t sent = 0x00;

    CHECK_FALSE(SolidSyslogBlockDevice_WriteAt(device, BLOCK_COUNT, 0, &sent, 1));
}

TEST(SolidSyslogNorFlashBlockDevice, AWriteAtFailsWhenTheProgramFails)
{
    const uint8_t sent = 0x00;
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    NorFlashFake_FailNextProgram();

    CHECK_FALSE(SolidSyslogBlockDevice_WriteAt(device, 0, 0, &sent, 1));
}

TEST(SolidSyslogNorFlashBlockDevice, ADisposedBlockDoesNotExist)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));

    CHECK_TRUE(SolidSyslogBlockDevice_Dispose(device, 0));

    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 0));
}

TEST(SolidSyslogNorFlashBlockDevice, DisposeErasesTheBlockAndMarksItErased)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "hello"));
    NorFlashFake_ResetCounts();

    CHECK_TRUE(SolidSyslogBlockDevice_Dispose(device, 0));

    LONGS_EQUAL(1, NorFlashFake_EraseCount());
    CHECK_BLOCK_STATE(0, 0x5A, 0xFF, 0xFF, 0xFF);
    BYTES_EQUAL(0xFF, NorFlashFake_Peek(FIRST_BLOCK, STATE_SIZE));
}

TEST(SolidSyslogNorFlashBlockDevice, AcquiringABlockDisposeLeftErasedCostsOneProgramAndNoErase)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(SolidSyslogBlockDevice_Dispose(device, 0));
    NorFlashFake_ResetCounts();

    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));

    LONGS_EQUAL(0, NorFlashFake_EraseCount());
    LONGS_EQUAL(1, NorFlashFake_ProgramCount());
}

TEST(SolidSyslogNorFlashBlockDevice, ABlockIsMarkedDisposedBeforeItIsErased)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    NorFlashFake_CutPowerAt(2, NORFLASHFAKE_CUT_CLEAN); /* the erase */

    CHECK_FALSE(SolidSyslogBlockDevice_Dispose(device, 0));

    CHECK_BLOCK_STATE(0, 0x00, 0x00, 0x00, 0x00);
}

TEST(SolidSyslogNorFlashBlockDevice, ABlockWhoseDisposeLostPowerBeforeTheEraseDoesNotExist)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "hello"));
    NorFlashFake_CutPowerAt(2, NORFLASHFAKE_CUT_CLEAN);
    CHECK_FALSE(SolidSyslogBlockDevice_Dispose(device, 0));
    NorFlashFake_PowerOn();

    Restart();

    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 0));
}

TEST(SolidSyslogNorFlashBlockDevice, ABlockWhoseEraseWasCutShortDoesNotExist)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "hello"));
    NorFlashFake_CutPowerAt(2, NORFLASHFAKE_CUT_TORN);
    CHECK_FALSE(SolidSyslogBlockDevice_Dispose(device, 0));
    NorFlashFake_PowerOn();

    Restart();

    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 0));
}

/* Its state reads as blank, and the cells past where the erase reached still
 * hold what was there. */
TEST(SolidSyslogNorFlashBlockDevice, ABlockWhoseEraseWasCutShortIsErasedAgainWhenItIsAcquired)
{
    char whole[DATA_BYTES];
    memset(whole, 'x', sizeof(whole));
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(SolidSyslogBlockDevice_Append(device, 0, whole, sizeof(whole)));
    NorFlashFake_CutPowerAt(2, NORFLASHFAKE_CUT_TORN);
    CHECK_FALSE(SolidSyslogBlockDevice_Dispose(device, 0));
    NorFlashFake_PowerOn();
    Restart();

    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));

    CHECK_BLOCK_SIZE(0, 0);
    CHECK_TRUE(Append(0, "hello"));
}

TEST(SolidSyslogNorFlashBlockDevice, DisposeFailsWhenTheBlockCannotBeMarkedDisposed)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    NorFlashFake_FailNextProgram();

    CHECK_FALSE(SolidSyslogBlockDevice_Dispose(device, 0));

    CHECK_TRUE(SolidSyslogBlockDevice_Exists(device, 0));
}

TEST(SolidSyslogNorFlashBlockDevice, DisposeFailsWhenTheEraseFails)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    NorFlashFake_FailNextErase();

    CHECK_FALSE(SolidSyslogBlockDevice_Dispose(device, 0));
}

TEST(SolidSyslogNorFlashBlockDevice, DisposeFailsWhenTheBlockCannotBeMarkedErased)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    NorFlashFake_CutPowerAt(3, NORFLASHFAKE_CUT_CLEAN); /* the second program */

    CHECK_FALSE(SolidSyslogBlockDevice_Dispose(device, 0));
}

TEST(SolidSyslogNorFlashBlockDevice, DisposingABlockPastTheRunFails)
{
    CHECK_FALSE(SolidSyslogBlockDevice_Dispose(device, BLOCK_COUNT));

    LONGS_EQUAL(0, NorFlashFake_WriteOperationCount());
}

TEST(SolidSyslogNorFlashBlockDevice, DisposingOneBlockLeavesTheEndOfAnotherKnown)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 1));
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "hello"));
    CHECK_TRUE(SolidSyslogBlockDevice_Dispose(device, 1));
    NorFlashFake_ResetCounts();

    CHECK_TRUE(Append(0, "world"));

    LONGS_EQUAL(1, NorFlashFake_ReadCount());
}

TEST(SolidSyslogNorFlashBlockDevice, ABlockAcquiredAgainAfterADisposeStartsEmpty)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "hello"));
    CHECK_TRUE(SolidSyslogBlockDevice_Dispose(device, 0));
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));

    CHECK_TRUE(Append(0, "world"));

    CHECK_BLOCK_CONTAINS(0, 0, "world", 5);
    CHECK_BLOCK_SIZE(0, 5);
}

TEST(SolidSyslogNorFlashBlockDevice, AnAcquireThatFailsForgetsWhereTheBlockEnded)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(Append(0, "hello"));
    NorFlashFake_FailNextErase();
    CHECK_FALSE(SolidSyslogBlockDevice_Acquire(device, 0));
    NorFlashFake_ResetCounts();

    CHECK_TRUE(Append(0, "world"));

    CHECK_BLOCK_CONTAINS(0, 0, "helloworld", 10);
    CHECK_TRUE(NorFlashFake_ReadCount() > 1U); /* it looked for the end */
}

TEST(SolidSyslogNorFlashBlockDevice, ADestroyedDeviceIsASafeNoOp)
{
    struct SolidSyslogBlockDevice* destroyed = device;
    DestroyDevice();

    CHECK_FALSE(SolidSyslogBlockDevice_Acquire(destroyed, 0));
    LONGS_EQUAL(0, SolidSyslogBlockDevice_GetBlockCount(destroyed));
}

TEST(SolidSyslogNorFlashBlockDevice, DestroyLeavesTheFlashAsItWas)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    NorFlashFake_ResetCounts();
    size_t writesBefore = NorFlashFake_WriteOperationCount();

    DestroyDevice();

    LONGS_EQUAL(writesBefore, NorFlashFake_WriteOperationCount());
    LONGS_EQUAL(0, NorFlashFake_ReadCount());
}

// clang-format off
TEST_GROUP_BASE(SolidSyslogNorFlashBlockDevicePool, NorFlashBlockDeviceTestBase)
{
    std::array<struct SolidSyslogBlockDevice*, SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_POOL_SIZE> pooled = {};
    struct SolidSyslogBlockDevice* overflow = nullptr;

    void setup() override
    {
        SetupFlash();
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogNorFlashBlockDevice_Destroy(handle);
            }
        }
    }

    struct SolidSyslogBlockDevice* MakeDevice()
    {
        CreateDevice();
        struct SolidSyslogBlockDevice* made = device;
        device = nullptr;
        return made;
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

TEST(SolidSyslogNorFlashBlockDevicePool, OverflowReportsPoolExhausted)
{
    FillPool();
    ErrorHandlerFake_Install(nullptr);

    overflow = MakeDevice();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogNorFlashBlockDeviceErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogNorFlashBlockDevicePool, OverflowGivesTheNullDevice)
{
    FillPool();

    overflow = MakeDevice();

    POINTERS_EQUAL(SolidSyslogNullBlockDevice_Get(), overflow);
}

TEST(SolidSyslogNorFlashBlockDevicePool, ADestroyedSlotIsCreatedInAgain)
{
    FillPool();
    SolidSyslogNorFlashBlockDevice_Destroy(pooled[0]);

    pooled[0] = MakeDevice();

    CHECK_TRUE(pooled[0] != SolidSyslogNullBlockDevice_Get());
}

TEST(SolidSyslogNorFlashBlockDevicePool, DestroyingAHandleTwiceReportsTheSecond)
{
    pooled[0] = MakeDevice();
    SolidSyslogNorFlashBlockDevice_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogNorFlashBlockDevice_Destroy(pooled[0]);

    pooled[0] = nullptr;
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
        &SolidSyslogNorFlashBlockDeviceErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogNorFlashBlockDevicePool, DestroyingAHandleThatIsNotADeviceIsReported)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogNorFlashBlockDevice_Destroy(SolidSyslogNullBlockDevice_Get());

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
        &SolidSyslogNorFlashBlockDeviceErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_UNKNOWN_DESTROY
    );
}
