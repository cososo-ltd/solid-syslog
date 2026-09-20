#include "LittleFsDisk.h"

#include <string.h>

static struct LittleFsDisk* DiskFromConfig(const struct lfs_config* config);
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
    memcpy(buffer, &disk->Storage[(block * LITTLEFS_DISK_BLOCK_SIZE) + off], size);
    return LFS_ERR_OK;
}

static int DiskProg(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size)
{
    struct LittleFsDisk* disk = DiskFromConfig(c);
    uint8_t* target = &disk->Storage[(block * LITTLEFS_DISK_BLOCK_SIZE) + off];
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
    int result = LFS_ERR_OK;
    if (WriteIsCut(disk))
    {
        result = LFS_ERR_IO;
    }
    else
    {
        memset(&disk->Storage[block * LITTLEFS_DISK_BLOCK_SIZE], 0xFF, LITTLEFS_DISK_BLOCK_SIZE);
    }
    return result;
}

static int DiskSync(const struct lfs_config* c)
{
    return DiskFromConfig(c)->PowerLost ? LFS_ERR_IO : LFS_ERR_OK;
}
