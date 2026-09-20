/* LittleFS block device over SemihostingDisk - see LfsSemihostingDisk.h. */

#include "LfsSemihostingDisk.h"

#include "SemihostingDisk.h"

#include <string.h>

/* What an erased byte reads as. LittleFS programs bits down from this. */
#define ERASED_BYTE 0xFFU

enum
{
    SECTORS_PER_BLOCK = LFS_SEMIHOSTING_BLOCK_SIZE / SEMIHOSTING_DISK_SECTOR_SIZE
};

static uint8_t readBuffer[LFS_SEMIHOSTING_CACHE_SIZE];
static uint8_t progBuffer[LFS_SEMIHOSTING_CACHE_SIZE];
static uint8_t lookaheadBuffer[LFS_SEMIHOSTING_LOOKAHEAD_SIZE];
static uint8_t erasePattern[SEMIHOSTING_DISK_SECTOR_SIZE];

static int DiskRead(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);
static int DiskProg(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size);
static int DiskErase(const struct lfs_config* c, lfs_block_t block);
static int DiskSync(const struct lfs_config* c);

static uint32_t SectorFor(lfs_block_t block, lfs_off_t off);
static int TransferResult(enum SemihostingDiskResult result);

static const struct lfs_config CONFIG = {
    .context = NULL,
    .read = DiskRead,
    .prog = DiskProg,
    .erase = DiskErase,
    .sync = DiskSync,
    .read_size = LFS_SEMIHOSTING_READ_SIZE,
    .prog_size = LFS_SEMIHOSTING_PROG_SIZE,
    .block_size = LFS_SEMIHOSTING_BLOCK_SIZE,
    .block_count = LFS_SEMIHOSTING_BLOCK_COUNT,
    .block_cycles = 500,
    .cache_size = LFS_SEMIHOSTING_CACHE_SIZE,
    .lookahead_size = LFS_SEMIHOSTING_LOOKAHEAD_SIZE,
    .read_buffer = readBuffer,
    .prog_buffer = progBuffer,
    .lookahead_buffer = lookaheadBuffer,
};

const struct lfs_config* LfsSemihostingDisk_Config(void)
{
    return &CONFIG;
}

/* Reads and writes are whole sectors: read_size and prog_size are one sector,
 * and LittleFS never asks for less than those. */
static uint32_t SectorFor(lfs_block_t block, lfs_off_t off)
{
    return (uint32_t) ((block * (lfs_block_t) SECTORS_PER_BLOCK) + (off / SEMIHOSTING_DISK_SECTOR_SIZE));
}

static int TransferResult(enum SemihostingDiskResult result)
{
    return (result == SEMIHOSTING_DISK_OK) ? LFS_ERR_OK : LFS_ERR_IO;
}

static int DiskRead(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size)
{
    (void) c;
    if (!SemihostingDisk_EnsureReady())
    {
        return LFS_ERR_IO;
    }
    uint32_t count = size / SEMIHOSTING_DISK_SECTOR_SIZE;
    return TransferResult(SemihostingDisk_Read(buffer, SectorFor(block, off), count));
}

static int DiskProg(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size)
{
    (void) c;
    if (!SemihostingDisk_EnsureReady())
    {
        return LFS_ERR_IO;
    }
    uint32_t count = size / SEMIHOSTING_DISK_SECTOR_SIZE;
    return TransferResult(SemihostingDisk_Write(buffer, SectorFor(block, off), count));
}

/* The image is a flat disk with no erase of its own, so the block is written
 * back as erased one sector at a time. */
static int DiskErase(const struct lfs_config* c, lfs_block_t block)
{
    (void) c;
    int result = LFS_ERR_OK;
    if (!SemihostingDisk_EnsureReady())
    {
        return LFS_ERR_IO;
    }
    /* Filled here rather than once at start-up, so the driver carries no
     * ordering assumption about having been configured first. */
    memset(erasePattern, ERASED_BYTE, sizeof(erasePattern));
    for (uint32_t sector = 0; (sector < (uint32_t) SECTORS_PER_BLOCK) && (result == LFS_ERR_OK); sector++)
    {
        result = TransferResult(SemihostingDisk_Write(erasePattern, SectorFor(block, 0) + sector, 1));
    }
    return result;
}

/* Every write reaches the host image through the semihosting trap as it is
 * made, so there is nothing held back to flush. */
static int DiskSync(const struct lfs_config* c)
{
    (void) c;
    return LFS_ERR_OK;
}
