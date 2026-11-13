/* The NUCLEO-F767ZI's disk behind the board seam - see BddTargetBoardDisk.h.
 *
 * No media is fitted, so the disk never becomes ready. A mount over it fails,
 * and the target stays on the store it started with. */

#include "BddTargetBoardDisk.h"

bool BddTargetBoardDisk_EnsureReady(void)
{
    return false;
}

bool BddTargetBoardDisk_IsReady(void)
{
    return false;
}

enum BddTargetBoardDiskResult BddTargetBoardDisk_Read(void* buffer, uint32_t sector, uint32_t count)
{
    (void) buffer;
    (void) sector;
    (void) count;
    return BDD_TARGET_BOARD_DISK_IO_ERROR;
}

enum BddTargetBoardDiskResult BddTargetBoardDisk_Write(const void* buffer, uint32_t sector, uint32_t count)
{
    (void) buffer;
    (void) sector;
    (void) count;
    return BDD_TARGET_BOARD_DISK_IO_ERROR;
}
