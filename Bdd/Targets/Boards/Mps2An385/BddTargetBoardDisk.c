/* The QEMU mps2-an385 machine's disk - see BddTargetBoardDisk.h. The machine
 * has no storage of its own, so the disk is a flat image on the QEMU host:
 * solidsyslog-disk.img in QEMU's working directory.
 *
 * Behave's after_scenario removes the image so each scenario starts with no
 * filesystem. EnsureReady creates a fresh sparse file when the open fails or
 * the file is too small, so the filesystem's mount then sees a zero-filled
 * image and the target falls through to format-on-first-use. */

#include "BddTargetBoardDisk.h"

#include "SemihostingFile.h"

enum
{
    DISK_TOTAL_BYTES = BDD_TARGET_BOARD_DISK_SECTOR_COUNT * BDD_TARGET_BOARD_DISK_SECTOR_SIZE,
};

static const char DISK_IMAGE_PATH[] = "solidsyslog-disk.img";

static int g_diskHandle = -1;

static bool BddTargetBoardDisk_IsRangeValid(uint32_t sector, uint32_t count);

bool BddTargetBoardDisk_EnsureReady(void)
{
    if (g_diskHandle < 0)
    {
        g_diskHandle = SemihostingFile_OpenImage(DISK_IMAGE_PATH, (int) DISK_TOTAL_BYTES);
    }
    return g_diskHandle >= 0;
}

bool BddTargetBoardDisk_IsReady(void)
{
    return g_diskHandle >= 0;
}

enum BddTargetBoardDiskResult BddTargetBoardDisk_Read(void* buffer, uint32_t sector, uint32_t count)
{
    enum BddTargetBoardDiskResult result = BDD_TARGET_BOARD_DISK_OK;
    if (!BddTargetBoardDisk_IsRangeValid(sector, count))
    {
        result = BDD_TARGET_BOARD_DISK_OUT_OF_RANGE;
    }
    else
    {
        int position = (int) sector * (int) BDD_TARGET_BOARD_DISK_SECTOR_SIZE;
        int bytes = (int) count * (int) BDD_TARGET_BOARD_DISK_SECTOR_SIZE;
        if (SemihostingFile_ReadAt(g_diskHandle, position, buffer, bytes) != 0)
        {
            result = BDD_TARGET_BOARD_DISK_IO_ERROR;
        }
    }
    return result;
}

/* Range-checks an LBA span without overflow: the subtraction form keeps the sum
 * `sector + count` from wrapping in uint32 (an arithmetic add could wrap a huge
 * sector past the comparison and admit an out-of-bounds seek). */
static bool BddTargetBoardDisk_IsRangeValid(uint32_t sector, uint32_t count)
{
    return (count <= (uint32_t) BDD_TARGET_BOARD_DISK_SECTOR_COUNT) &&
           (sector <= ((uint32_t) BDD_TARGET_BOARD_DISK_SECTOR_COUNT - count));
}

/* The image is a file on the QEMU host, and the harness removes it to start
 * with an empty disk. */
bool BddTargetBoardDisk_Wipe(void)
{
    return false;
}

enum BddTargetBoardDiskResult BddTargetBoardDisk_Write(const void* buffer, uint32_t sector, uint32_t count)
{
    enum BddTargetBoardDiskResult result = BDD_TARGET_BOARD_DISK_OK;
    if (!BddTargetBoardDisk_IsRangeValid(sector, count))
    {
        result = BDD_TARGET_BOARD_DISK_OUT_OF_RANGE;
    }
    else
    {
        int position = (int) sector * (int) BDD_TARGET_BOARD_DISK_SECTOR_SIZE;
        int bytes = (int) count * (int) BDD_TARGET_BOARD_DISK_SECTOR_SIZE;
        if (SemihostingFile_WriteAt(g_diskHandle, position, buffer, bytes) != 0)
        {
            result = BDD_TARGET_BOARD_DISK_IO_ERROR;
        }
    }
    return result;
}
