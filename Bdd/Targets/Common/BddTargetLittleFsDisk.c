/* LittleFS block device over the board's disk - see BddTargetLittleFsDisk.h. */

#include "BddTargetLittleFsDisk.h"

#include "BddTargetBoardDisk.h"

#include <string.h>

/* What an erased byte reads as. LittleFS programs bits down from this. */
#define ERASED_BYTE 0xFFU

enum
{
    SECTORS_PER_BLOCK = BDD_TARGET_LITTLE_FS_DISK_BLOCK_SIZE / BDD_TARGET_BOARD_DISK_SECTOR_SIZE
};

static uint8_t readBuffer[BDD_TARGET_LITTLE_FS_DISK_CACHE_SIZE];
static uint8_t progBuffer[BDD_TARGET_LITTLE_FS_DISK_CACHE_SIZE];
static uint8_t lookaheadBuffer[BDD_TARGET_LITTLE_FS_DISK_LOOKAHEAD_SIZE];
static uint8_t erasePattern[BDD_TARGET_BOARD_DISK_SECTOR_SIZE];

static int DiskRead(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);
static int DiskProg(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size);
static int DiskErase(const struct lfs_config* c, lfs_block_t block);
static int DiskSync(const struct lfs_config* c);

static uint32_t SectorFor(lfs_block_t block, lfs_off_t off);
static int TransferResult(enum BddTargetBoardDiskResult result);

static const struct lfs_config CONFIG = {
    .context = NULL,
    .read = DiskRead,
    .prog = DiskProg,
    .erase = DiskErase,
    .sync = DiskSync,
    .read_size = BDD_TARGET_LITTLE_FS_DISK_READ_SIZE,
    .prog_size = BDD_TARGET_LITTLE_FS_DISK_PROG_SIZE,
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

/* Reads and writes are whole sectors: read_size and prog_size are one sector,
 * and LittleFS never asks for less than those. */
static uint32_t SectorFor(lfs_block_t block, lfs_off_t off)
{
    return (uint32_t) ((block * (lfs_block_t) SECTORS_PER_BLOCK) + (off / BDD_TARGET_BOARD_DISK_SECTOR_SIZE));
}

static int TransferResult(enum BddTargetBoardDiskResult result)
{
    return (result == BDD_TARGET_BOARD_DISK_OK) ? LFS_ERR_OK : LFS_ERR_IO;
}

static int DiskRead(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size)
{
    (void) c;
    if (!BddTargetBoardDisk_EnsureReady())
    {
        return LFS_ERR_IO;
    }
    uint32_t count = size / BDD_TARGET_BOARD_DISK_SECTOR_SIZE;
    return TransferResult(BddTargetBoardDisk_Read(buffer, SectorFor(block, off), count));
}

static int DiskProg(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size)
{
    (void) c;
    if (!BddTargetBoardDisk_EnsureReady())
    {
        return LFS_ERR_IO;
    }
    uint32_t count = size / BDD_TARGET_BOARD_DISK_SECTOR_SIZE;
    return TransferResult(BddTargetBoardDisk_Write(buffer, SectorFor(block, off), count));
}

/* The image is a flat disk with no erase of its own, so the block is written
 * back as erased one sector at a time. */
static int DiskErase(const struct lfs_config* c, lfs_block_t block)
{
    (void) c;
    int result = LFS_ERR_OK;
    if (!BddTargetBoardDisk_EnsureReady())
    {
        return LFS_ERR_IO;
    }
    /* Filled here rather than once at start-up, so the driver carries no
     * ordering assumption about having been configured first. */
    memset(erasePattern, ERASED_BYTE, sizeof(erasePattern));
    for (uint32_t sector = 0U; (sector < (uint32_t) SECTORS_PER_BLOCK) && (result == LFS_ERR_OK); sector++)
    {
        result = TransferResult(BddTargetBoardDisk_Write(erasePattern, SectorFor(block, 0) + sector, 1));
    }
    return result;
}

/* BddTargetBoardDisk_Write returns with the sectors written, so there is
 * nothing held back here to flush. */
static int DiskSync(const struct lfs_config* c)
{
    (void) c;
    return LFS_ERR_OK;
}
