#include "BddTargetBoardSpiNorFlashBus.h"
#include "BddTargetCountingNorFlash.h"
#include "BddTargetNorFlashStore.h"
#include "BddTargetStoreSettings.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogNorFlashBlockDevice.h"
#include "SolidSyslogNullBlockDevice.h"
#include "SpiNorFlashPartFake.h"
#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

enum
{
    /* The fake part's smallest erase, which is a block of the flash. */
    FLASH_BLOCK_BYTES = 4096,
    FLASH_BLOCK_DATA_BYTES = FLASH_BLOCK_BYTES - SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_STATE_SIZE,

    FIRST_BLOCK = 0,
    LAST_BLOCK = BDD_TARGET_NOR_FLASH_STORE_BLOCK_COUNT - 1,

    COUNTS_BYTES = 80
};

/* The board seam the store takes its bus from. Here the board is the fake
 * part, a W25Q128JV until a test says otherwise. */
struct SolidSyslogSpiNorFlashBus* BddTargetBoardSpiNorFlashBus_Take(void)
{
    return SpiNorFlashPartFake::Bus();
}

// clang-format off
TEST_GROUP(BddTargetNorFlashStore)
{
    struct SolidSyslogBlockDevice* device = nullptr;

    void setup() override
    {
        SpiNorFlashPartFake::Reset();
        ErrorHandlerFake_Install(nullptr);
        BddTargetStoreSettings_Reset();
    }

    void teardown() override
    {
        DestroyDevice();
        BddTargetNorFlashStore_Unmount();
        /* The fake holds what it was sent in memory the leak check counts. */
        SpiNorFlashPartFake::Release();
    }

    void CreateDevice()
    {
        device = BddTargetNorFlashStore_CreateBlockDevice();
    }

    /* Leaves the part holding one block of a store, with no device over it. */
    void MountAndAcquire(size_t block)
    {
        BddTargetNorFlashStore_Mount();
        CreateDevice();
        SolidSyslogBlockDevice_Acquire(device, block);
        DestroyDevice();
    }

    void DestroyDevice()
    {
        if (device != nullptr)
        {
            BddTargetNorFlashStore_DestroyBlockDevice(device);
            device = nullptr;
        }
    }
};

// clang-format on

TEST(BddTargetNorFlashStore, MountStartsThePart)
{
    CHECK_TRUE(BddTargetNorFlashStore_Mount());
}

TEST(BddTargetNorFlashStore, MountAnswersFalseForAPartThatIsNotThere)
{
    SpiNorFlashPartFake::settings.present = false;

    CHECK_FALSE(BddTargetNorFlashStore_Mount());
}

TEST(BddTargetNorFlashStore, AMountThatFailedIsTriedAgain)
{
    SpiNorFlashPartFake::settings.present = false;
    BddTargetNorFlashStore_Mount();
    SpiNorFlashPartFake::settings.present = true;

    CHECK_TRUE(BddTargetNorFlashStore_Mount());
}

TEST(BddTargetNorFlashStore, UnmountLetsGoOfThePart)
{
    BddTargetNorFlashStore_Mount();

    BddTargetNorFlashStore_Unmount();

    SpiNorFlashPartFake::settings.present = false;
    CHECK_FALSE(BddTargetNorFlashStore_Mount());
}

TEST(BddTargetNorFlashStore, UnmountWithNoPartStartedReportsNothing)
{
    BddTargetNorFlashStore_Unmount();

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(BddTargetNorFlashStore, TheBlockDeviceHasTheStoresRunOfBlocks)
{
    BddTargetNorFlashStore_Mount();

    CreateDevice();

    LONGS_EQUAL(BDD_TARGET_NOR_FLASH_STORE_BLOCK_COUNT, SolidSyslogBlockDevice_GetBlockCount(device));
}

TEST(BddTargetNorFlashStore, ABlockIsAllAFlashBlockHoldsWhenNoSizeIsSet)
{
    BddTargetNorFlashStore_Mount();

    CreateDevice();

    LONGS_EQUAL(FLASH_BLOCK_DATA_BYTES, SolidSyslogBlockDevice_GetBlockSize(device));
}

TEST(BddTargetNorFlashStore, ABlockIsTheSizeTheHarnessSet)
{
    BddTargetNorFlashStore_Mount();
    BddTargetStoreSettings_SetByName("max-block-size", "520");

    CreateDevice();

    LONGS_EQUAL(520, SolidSyslogBlockDevice_GetBlockSize(device));
}

TEST(BddTargetNorFlashStore, ASizeMoreThanAFlashBlockHoldsIsRefused)
{
    BddTargetNorFlashStore_Mount();
    BddTargetStoreSettings_SetByName("max-block-size", "5000");

    CreateDevice();

    POINTERS_EQUAL(SolidSyslogNullBlockDevice_Get(), device);
}

TEST(BddTargetNorFlashStore, WipeStartsThePartItself)
{
    CHECK_TRUE(BddTargetNorFlashStore_Wipe());
}

TEST(BddTargetNorFlashStore, WipeAnswersFalseForAPartThatIsNotThere)
{
    SpiNorFlashPartFake::settings.present = false;

    CHECK_FALSE(BddTargetNorFlashStore_Wipe());
}

TEST(BddTargetNorFlashStore, WipeLeavesTheBlockDeviceForTheStore)
{
    BddTargetNorFlashStore_Wipe();

    CreateDevice();

    CHECK_TRUE(device != SolidSyslogNullBlockDevice_Get());
}

TEST(BddTargetNorFlashStore, WipeDisposesOfABlockThatExists)
{
    MountAndAcquire(FIRST_BLOCK);

    BddTargetNorFlashStore_Wipe();

    CreateDevice();
    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, FIRST_BLOCK));
}

TEST(BddTargetNorFlashStore, WipeReachesTheLastBlockOfTheRun)
{
    MountAndAcquire(LAST_BLOCK);

    BddTargetNorFlashStore_Wipe();

    CreateDevice();
    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, LAST_BLOCK));
}

TEST(BddTargetNorFlashStore, WipeErasesNothing)
{
    MountAndAcquire(FIRST_BLOCK);
    size_t erasesBefore = SpiNorFlashPartFake::Erases().size();

    BddTargetNorFlashStore_Wipe();

    LONGS_EQUAL(erasesBefore, SpiNorFlashPartFake::Erases().size());
}

TEST(BddTargetNorFlashStore, WipeAnswersFalseWhenABlockCannotBeDisposedOf)
{
    MountAndAcquire(FIRST_BLOCK);
    SpiNorFlashPartFake::settings.ignoresWriteEnable = true;

    CHECK_FALSE(BddTargetNorFlashStore_Wipe());
}

TEST(BddTargetNorFlashStore, WhatTheStoreAsksOfThePartIsCounted)
{
    char counts[COUNTS_BYTES] = {};
    MountAndAcquire(FIRST_BLOCK);

    BddTargetCountingNorFlash_Describe(counts, sizeof(counts));

    STRCMP_CONTAINS("erases 1,", counts);
}

TEST(BddTargetNorFlashStore, NothingIsCountedOnceThePartIsLetGo)
{
    char counts[COUNTS_BYTES] = {};
    BddTargetNorFlashStore_Mount();

    BddTargetNorFlashStore_Unmount();

    CHECK_FALSE(BddTargetCountingNorFlash_Describe(counts, sizeof(counts)));
}
