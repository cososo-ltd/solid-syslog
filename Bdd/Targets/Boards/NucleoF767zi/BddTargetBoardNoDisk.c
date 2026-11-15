/* The NUCLEO-F767ZI behind the board seam's disk, for a target the board has
 * no disk for yet - see BddTargetBoardDisk.h.
 *
 * The disk never becomes ready, so a mount over it fails and the target stays
 * on the store it started with. Board.cmake says which targets are given this
 * in place of the MicroSD card. */

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

bool BddTargetBoardDisk_Wipe(void)
{
    return false;
}
