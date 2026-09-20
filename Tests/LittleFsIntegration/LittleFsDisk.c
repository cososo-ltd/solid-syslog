#include "LittleFsDisk.h"

#include <stddef.h>
#include <string.h>

static struct LittleFsDisk* DiskFromConfig(const struct lfs_config* config);
static size_t ByteOffset(lfs_block_t block, lfs_off_t off);
static bool WriteIsCut(struct LittleFsDisk* disk);

static int DiskRead(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);
static int DiskProg(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size);
static int DiskErase(const struct lfs_config* c, lfs_block_t block);
static int DiskSync(const struct lfs_config* c);

void LittleFsDisk_Init(struct LittleFsDisk* disk)
{
    memset(disk->Storage, 0xFF, sizeof(disk->Storage));
    disk->WriteCount = 0U;
    disk->CutAfter = 0U;
    disk->CutKind = LITTLEFS_DISK_CUT_CLEAN;
    disk->PowerLost = false;
}

void LittleFsDisk_Configure(struct LittleFsDisk* disk, struct lfs_config* config)
{
    memset(config, 0, sizeof(*config));
    config->context = disk;
    config->read = DiskRead;
    config->prog = DiskProg;
    config->erase = DiskErase;
    config->sync = DiskSync;
    config->read_size = LITTLEFS_DISK_READ_SIZE;
    config->prog_size = LITTLEFS_DISK_PROG_SIZE;
    config->block_size = LITTLEFS_DISK_BLOCK_SIZE;
    config->block_count = LITTLEFS_DISK_BLOCK_COUNT;
    config->cache_size = LITTLEFS_DISK_CACHE_SIZE;
    config->lookahead_size = LITTLEFS_DISK_LOOKAHEAD_SIZE;
    /* Wear levelling on, as the platform page tells integrators to set it. */
    config->block_cycles = 500;
    config->read_buffer = disk->ReadBuffer;
    config->prog_buffer = disk->ProgBuffer;
    config->lookahead_buffer = disk->LookaheadBuffer;
}

void LittleFsDisk_CutAfter(struct LittleFsDisk* disk, unsigned writes, enum LittleFsDiskCut kind)
{
    disk->WriteCount = 0U;
    disk->CutAfter = writes;
    disk->CutKind = kind;
    disk->PowerLost = false;
}

bool LittleFsDisk_PowerLost(const struct LittleFsDisk* disk)
{
    return disk->PowerLost;
}

void LittleFsDisk_PowerOn(struct LittleFsDisk* disk)
{
    disk->PowerLost = false;
    disk->CutAfter = 0U;
    disk->WriteCount = 0U;
}

unsigned LittleFsDisk_WriteCount(const struct LittleFsDisk* disk)
{
    return disk->WriteCount;
}

static struct LittleFsDisk* DiskFromConfig(const struct lfs_config* c)
{
    return (struct LittleFsDisk*) c->context;
}

/* Widen before multiplying: the product is a byte offset, and computing it in
 * the block type first would overflow on a device large enough to need it. */
static size_t ByteOffset(lfs_block_t block, lfs_off_t off)
{
    return ((size_t) block * (size_t) LITTLEFS_DISK_BLOCK_SIZE) + (size_t) off;
}

/* Counts the attempt, then says whether the power is now gone. Once it is, it
 * stays gone until LittleFsDisk_PowerOn. */
static bool WriteIsCut(struct LittleFsDisk* disk)
{
    if (disk->PowerLost)
    {
        return true;
    }
    disk->WriteCount++;
    if ((disk->CutAfter != 0U) && (disk->WriteCount >= disk->CutAfter))
    {
        disk->PowerLost = true;
    }
    return disk->PowerLost;
}

static int DiskRead(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size)
{
    struct LittleFsDisk* disk = DiskFromConfig(c);
    if (disk->PowerLost)
    {
        return LFS_ERR_IO;
    }
    memcpy(buffer, &disk->Storage[ByteOffset(block, off)], size);
    return LFS_ERR_OK;
}

static int DiskProg(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size)
{
    struct LittleFsDisk* disk = DiskFromConfig(c);
    uint8_t* target = &disk->Storage[ByteOffset(block, off)];
    int result = LFS_ERR_OK;
    if (WriteIsCut(disk))
    {
        /* A torn program leaves half its bytes behind - the case LittleFS's
         * copy-on-write commits exist to survive. A clean one leaves none. */
        if (disk->CutKind == LITTLEFS_DISK_CUT_TORN)
        {
            memcpy(target, buffer, size / 2U);
        }
        result = LFS_ERR_IO;
    }
    else
    {
        memcpy(target, buffer, size);
    }
    return result;
}

static int DiskErase(const struct lfs_config* c, lfs_block_t block)
{
    struct LittleFsDisk* disk = DiskFromConfig(c);
    uint8_t* target = &disk->Storage[ByteOffset(block, 0)];
    int result = LFS_ERR_OK;
    if (WriteIsCut(disk))
    {
        /* A torn erase leaves the block half erased, which is as real a
         * power-cut state as a torn program and a different one to survive. */
        if (disk->CutKind == LITTLEFS_DISK_CUT_TORN)
        {
            memset(target, 0xFF, LITTLEFS_DISK_BLOCK_SIZE / 2U);
        }
        result = LFS_ERR_IO;
    }
    else
    {
        memset(target, 0xFF, LITTLEFS_DISK_BLOCK_SIZE);
    }
    return result;
}

static int DiskSync(const struct lfs_config* c)
{
    return DiskFromConfig(c)->PowerLost ? LFS_ERR_IO : LFS_ERR_OK;
}
