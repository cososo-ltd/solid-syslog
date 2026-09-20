#include "CppUTest/TestHarness.h"

extern "C"
{
#include "LittleFsDisk.h"
#include "lfs.h"
}

/* Every byte in [first, last) reads as `expected`. A macro rather than a
   function so a failure reports the line that asserted it. */
#define CHECK_BYTES_ARE(expected, first, last)                  \
    for (const uint8_t* byte = (first); byte != (last); ++byte) \
    {                                                           \
        BYTES_EQUAL((expected), *byte);                         \
    }

/* The emulated device is what every other test in this suite rests on. A cut
 * that never fired, or one that corrupted nothing, would make each of those
 * assertions pass for the wrong reason. */

static struct LittleFsDisk disk;
static struct lfs_config config;

// clang-format off
TEST_GROUP(LittleFsDisk)
{
    void setup() override
    {
        LittleFsDisk_Init(&disk);
        LittleFsDisk_Configure(&disk, &config);
    }

    static void ProgramOneUnit(uint8_t value)
    {
        uint8_t payload[LITTLEFS_DISK_PROG_SIZE];
        memset(payload, value, sizeof(payload));
        (void) config.prog(&config, 0, 0, payload, sizeof(payload));
    }
};

// clang-format on

TEST(LittleFsDisk, StartsErasedAsBlankFlashReads)
{
    uint8_t readBack[LITTLEFS_DISK_READ_SIZE] = {};

    LONGS_EQUAL(LFS_ERR_OK, config.read(&config, 0, 0, readBack, sizeof(readBack)));

    CHECK_BYTES_ARE(0xFF, readBack, readBack + sizeof(readBack));
}

TEST(LittleFsDisk, ProgramsAndReadsBackWhenThePowerIsOn)
{
    uint8_t readBack[LITTLEFS_DISK_PROG_SIZE] = {};

    ProgramOneUnit(0xA5);

    LONGS_EQUAL(LFS_ERR_OK, config.read(&config, 0, 0, readBack, sizeof(readBack)));
    CHECK_BYTES_ARE(0xA5, readBack, readBack + sizeof(readBack));
    CHECK_FALSE(LittleFsDisk_PowerLost(&disk));
}

TEST(LittleFsDisk, ACleanCutWritesNothingAndFails)
{
    uint8_t readBack[LITTLEFS_DISK_PROG_SIZE] = {};
    LittleFsDisk_CutAfter(&disk, 1, LITTLEFS_DISK_CUT_CLEAN);

    ProgramOneUnit(0xA5);

    CHECK_TRUE(LittleFsDisk_PowerLost(&disk));
    LittleFsDisk_PowerOn(&disk);
    LONGS_EQUAL(LFS_ERR_OK, config.read(&config, 0, 0, readBack, sizeof(readBack)));
    CHECK_BYTES_ARE(0xFF, readBack, readBack + sizeof(readBack));
}

TEST(LittleFsDisk, ATornCutLeavesHalfTheBytesBehind)
{
    uint8_t readBack[LITTLEFS_DISK_PROG_SIZE] = {};
    LittleFsDisk_CutAfter(&disk, 1, LITTLEFS_DISK_CUT_TORN);

    ProgramOneUnit(0xA5);

    CHECK_TRUE(LittleFsDisk_PowerLost(&disk));
    LittleFsDisk_PowerOn(&disk);
    LONGS_EQUAL(LFS_ERR_OK, config.read(&config, 0, 0, readBack, sizeof(readBack)));
    const uint8_t* untouched = readBack + (LITTLEFS_DISK_PROG_SIZE / 2U);
    CHECK_BYTES_ARE(0xA5, readBack, untouched);
    CHECK_BYTES_ARE(0xFF, untouched, readBack + LITTLEFS_DISK_PROG_SIZE);
}

TEST(LittleFsDisk, TheCutFiresOnTheArmedWriteAndNotBefore)
{
    LittleFsDisk_CutAfter(&disk, 3, LITTLEFS_DISK_CUT_CLEAN);

    ProgramOneUnit(0x11);
    CHECK_FALSE(LittleFsDisk_PowerLost(&disk));
    ProgramOneUnit(0x22);
    CHECK_FALSE(LittleFsDisk_PowerLost(&disk));
    ProgramOneUnit(0x33);

    CHECK_TRUE(LittleFsDisk_PowerLost(&disk));
}

TEST(LittleFsDisk, EverythingAfterTheCutAlsoFails)
{
    LittleFsDisk_CutAfter(&disk, 1, LITTLEFS_DISK_CUT_CLEAN);
    ProgramOneUnit(0xA5);

    uint8_t payload[LITTLEFS_DISK_PROG_SIZE] = {};
    LONGS_EQUAL(LFS_ERR_IO, config.prog(&config, 1, 0, payload, sizeof(payload)));
    LONGS_EQUAL(LFS_ERR_IO, config.erase(&config, 1));
    LONGS_EQUAL(LFS_ERR_IO, config.sync(&config));
    LONGS_EQUAL(LFS_ERR_IO, config.read(&config, 1, 0, payload, sizeof(payload)));
}

TEST(LittleFsDisk, ATornCutLeavesTheBlockHalfErased)
{
    uint8_t written[LITTLEFS_DISK_PROG_SIZE] = {};
    /* Fill the first two program units so both halves of the block start
       programmed, then tear the erase across them. */
    ProgramOneUnit(0x5A);
    uint8_t payload[LITTLEFS_DISK_PROG_SIZE];
    memset(payload, 0x5A, sizeof(payload));
    (void) config.prog(&config, 0, LITTLEFS_DISK_BLOCK_SIZE / 2U, payload, sizeof(payload));

    LittleFsDisk_CutAfter(&disk, 1, LITTLEFS_DISK_CUT_TORN);
    LONGS_EQUAL(LFS_ERR_IO, config.erase(&config, 0));

    CHECK_TRUE(LittleFsDisk_PowerLost(&disk));
    LittleFsDisk_PowerOn(&disk);
    /* The first half came back to erased; the second half did not. */
    LONGS_EQUAL(LFS_ERR_OK, config.read(&config, 0, 0, written, sizeof(written)));
    CHECK_BYTES_ARE(0xFF, written, written + sizeof(written));
    LONGS_EQUAL(LFS_ERR_OK, config.read(&config, 0, LITTLEFS_DISK_BLOCK_SIZE / 2U, written, sizeof(written)));
    CHECK_BYTES_ARE(0x5A, written, written + sizeof(written));
}

TEST(LittleFsDisk, ErasesCountTowardsTheCut)
{
    LittleFsDisk_CutAfter(&disk, 1, LITTLEFS_DISK_CUT_CLEAN);

    LONGS_EQUAL(LFS_ERR_IO, config.erase(&config, 0));

    CHECK_TRUE(LittleFsDisk_PowerLost(&disk));
}

TEST(LittleFsDisk, RealLittleFsFormatsAndMountsOnIt)
{
    lfs_t filesystem;

    LONGS_EQUAL(LFS_ERR_OK, lfs_format(&filesystem, &config));
    LONGS_EQUAL(LFS_ERR_OK, lfs_mount(&filesystem, &config));
    LONGS_EQUAL(LFS_ERR_OK, lfs_unmount(&filesystem));
    CHECK_TRUE(LittleFsDisk_WriteCount(&disk) > 0U);
}
