/* LittleFS block device over a SPI NOR flash - see BddTargetLittleFsDisk.h.
 *
 * A filesystem block is one of the part's sectors, so LittleFS's erase is the
 * part's own, and its programs reach the part as they are: they clear bits
 * and set none, which is what LittleFS is written for.
 *
 * The filesystem is given the first BDD_TARGET_LITTLE_FS_DISK_BLOCK_COUNT
 * sectors of the part, whatever the part's size.
 *
 * What goes wrong is said on the console, so that a part that has stopped
 * answering is told from a store that has a fault of its own. */

#include "BddTargetLittleFsDisk.h"

#include "BddTargetSpiNorFlash.h"

#include <stdbool.h>
#include <stdio.h>

_Static_assert(
    (int) BDD_TARGET_LITTLE_FS_DISK_BLOCK_SIZE == (int) BDD_TARGET_SPI_NOR_FLASH_SECTOR_SIZE,
    "a LittleFS block is one sector of the flash"
);

enum
{
    /* The part reads from any byte, for any length. */
    READ_SIZE = 1,
    /* The part programs any part of a page. LittleFS ends every commit on a
     * multiple of this, so a small one lets a metadata block take many
     * commits before it has to be compacted and erased. */
    PROG_SIZE = 16,
    FIRST_SUPERBLOCK = 0,
    SECOND_SUPERBLOCK = 1
};

static uint8_t readBuffer[BDD_TARGET_LITTLE_FS_DISK_CACHE_SIZE];
static uint8_t progBuffer[BDD_TARGET_LITTLE_FS_DISK_CACHE_SIZE];
static uint8_t lookaheadBuffer[BDD_TARGET_LITTLE_FS_DISK_LOOKAHEAD_SIZE];

/* What starting the part last came to, so that a mount tried again and again
 * over a part that is not there says so once. */
static enum BddTargetSpiNorFlashResult startFailureSaid = BDD_TARGET_SPI_NOR_FLASH_OK;
static bool tooSmallSaid;

static int FlashRead(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);
static int FlashProg(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size);
static int FlashErase(const struct lfs_config* c, lfs_block_t block);
static int FlashSync(const struct lfs_config* c);

static bool LittleFsFlash_EnsureReady(void);
static inline const char* LittleFsFlash_Reason(enum BddTargetSpiNorFlashResult result);
static inline uint32_t LittleFsFlash_AddressOf(lfs_block_t block, lfs_off_t off);
static int LittleFsFlash_Judge(const char* what, lfs_block_t block, enum BddTargetSpiNorFlashResult result);

static const struct lfs_config CONFIG = {
    .context = NULL,
    .read = FlashRead,
    .prog = FlashProg,
    .erase = FlashErase,
    .sync = FlashSync,
    .read_size = READ_SIZE,
    .prog_size = PROG_SIZE,
    .block_size = BDD_TARGET_LITTLE_FS_DISK_BLOCK_SIZE,
    .block_count = BDD_TARGET_LITTLE_FS_DISK_BLOCK_COUNT,
    .block_cycles = 500,
    .cache_size = BDD_TARGET_LITTLE_FS_DISK_CACHE_SIZE,
    .lookahead_size = BDD_TARGET_LITTLE_FS_DISK_LOOKAHEAD_SIZE,
    .read_buffer = readBuffer,
    .prog_buffer = progBuffer,
    .lookahead_buffer = lookaheadBuffer,
};

const struct lfs_config* BddTargetLittleFsDisk_Config(void)
{
    return &CONFIG;
}

/* LittleFS finds a filesystem by the pair of blocks its superblock is kept
 * in, so with both erased it finds none, and the mount that finds none
 * formats. What the blocks after them held is erased as each is next used. */
bool BddTargetLittleFsDisk_Wipe(void)
{
    return LittleFsFlash_EnsureReady() && (FlashErase(&CONFIG, FIRST_SUPERBLOCK) == LFS_ERR_OK) &&
           (FlashErase(&CONFIG, SECOND_SUPERBLOCK) == LFS_ERR_OK);
}

static bool LittleFsFlash_EnsureReady(void)
{
    if (!BddTargetSpiNorFlash_IsStarted())
    {
        enum BddTargetSpiNorFlashResult result = BddTargetSpiNorFlash_Start();
        if (result != BDD_TARGET_SPI_NOR_FLASH_OK)
        {
            if (result != startFailureSaid)
            {
                (void) printf("[board] flash: the part did not start: %s\n", LittleFsFlash_Reason(result));
            }
        }
        else if (BddTargetSpiNorFlash_Capacity() <
                 ((uint32_t) BDD_TARGET_LITTLE_FS_DISK_BLOCK_COUNT * (uint32_t) BDD_TARGET_LITTLE_FS_DISK_BLOCK_SIZE))
        {
            if (!tooSmallSaid)
            {
                (void) printf("[board] flash: the part is smaller than the filesystem\n");
                tooSmallSaid = true;
            }
            BddTargetSpiNorFlash_Stop();
        }
        else
        {
            /* Started, and large enough. */
        }
        startFailureSaid = result;
    }
    return BddTargetSpiNorFlash_IsStarted();
}

static inline const char* LittleFsFlash_Reason(enum BddTargetSpiNorFlashResult result)
{
    const char* reason = "unknown";
    switch (result)
    {
        case BDD_TARGET_SPI_NOR_FLASH_OK:
            reason = "ok";
            break;
        case BDD_TARGET_SPI_NOR_FLASH_NOT_STARTED:
            reason = "not started";
            break;
        case BDD_TARGET_SPI_NOR_FLASH_OUT_OF_RANGE:
            reason = "out of range";
            break;
        case BDD_TARGET_SPI_NOR_FLASH_NO_ANSWER:
            reason = "no answer";
            break;
        case BDD_TARGET_SPI_NOR_FLASH_UNSUPPORTED:
            reason = "not a part this drives";
            break;
        case BDD_TARGET_SPI_NOR_FLASH_TIMED_OUT:
            reason = "timed out";
            break;
        case BDD_TARGET_SPI_NOR_FLASH_WRITE_REFUSED:
            reason = "would not enable writing";
            break;
        default:
            break;
    }
    return reason;
}

static int FlashRead(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size)
{
    (void) c;
    int result = LFS_ERR_IO;
    if (LittleFsFlash_EnsureReady())
    {
        result = LittleFsFlash_Judge(
            "read",
            block,
            BddTargetSpiNorFlash_Read(LittleFsFlash_AddressOf(block, off), buffer, size)
        );
    }
    return result;
}

static inline uint32_t LittleFsFlash_AddressOf(lfs_block_t block, lfs_off_t off)
{
    return (uint32_t) ((block * (lfs_block_t) BDD_TARGET_LITTLE_FS_DISK_BLOCK_SIZE) + off);
}

/* A part that has failed is forgotten, so that the next thing asked of it
 * starts it again: a start waits out whatever the part was left doing. */
static int LittleFsFlash_Judge(const char* what, lfs_block_t block, enum BddTargetSpiNorFlashResult result)
{
    if (result != BDD_TARGET_SPI_NOR_FLASH_OK)
    {
        (void) printf(
            "[board] flash: %s failed, block %lu: %s\n",
            what,
            (unsigned long) block,
            LittleFsFlash_Reason(result)
        );
        BddTargetSpiNorFlash_Stop();
    }
    return (result == BDD_TARGET_SPI_NOR_FLASH_OK) ? LFS_ERR_OK : LFS_ERR_IO;
}

static int FlashProg(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size)
{
    (void) c;
    int result = LFS_ERR_IO;
    if (LittleFsFlash_EnsureReady())
    {
        result = LittleFsFlash_Judge(
            "program",
            block,
            BddTargetSpiNorFlash_Program(LittleFsFlash_AddressOf(block, off), buffer, size)
        );
    }
    return result;
}

static int FlashErase(const struct lfs_config* c, lfs_block_t block)
{
    (void) c;
    int result = LFS_ERR_IO;
    if (LittleFsFlash_EnsureReady())
    {
        result = LittleFsFlash_Judge("erase", block, BddTargetSpiNorFlash_EraseSector((uint32_t) block));
    }
    return result;
}

/* The driver returns from a program or an erase once the part has finished
 * it, so there is nothing held back here to flush. */
static int FlashSync(const struct lfs_config* c)
{
    (void) c;
    return LFS_ERR_OK;
}
