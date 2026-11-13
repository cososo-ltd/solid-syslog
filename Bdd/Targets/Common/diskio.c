/* ChaN-FatFs disk_* glue over the board's disk (BddTargetBoardDisk.h).
 *
 * Maps FatFs's caller-facing disk_* contract onto the sector disk every
 * filesystem's media glue shares. The media and its geometry are the
 * board's, so the FreeRTOS-Plus-FAT FF_Disk_t driver (BddTargetPlusFatDisk.c)
 * exercises byte-for-byte the same disk.
 *
 * Used by the lwIP BDD target (FatFs-backed store); the FreeRTOS-Plus-TCP
 * target uses Plus-FAT instead. Single logical drive (pdrv 0); no exFAT, no
 * LFN. */

/* ff.h before diskio.h: diskio.h declares disk_* in terms of BYTE / UINT /
 * LBA_t / DWORD / WORD which are typedef'd in ff.h's integer headers. */
#include "ff.h"
#include "diskio.h"

#include "BddTargetBoardDisk.h"

#include <stdbool.h>

static bool Diskio_IsDriveReady(BYTE pdrv);

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0)
    {
        return STA_NOINIT;
    }
    return BddTargetBoardDisk_EnsureReady() ? 0 : STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0)
    {
        return STA_NOINIT;
    }
    return BddTargetBoardDisk_IsReady() ? 0 : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
    DRESULT result;
    if (!Diskio_IsDriveReady(pdrv))
    {
        result = RES_NOTRDY;
    }
    else
    {
        switch (BddTargetBoardDisk_Read(buff, (uint32_t) sector, (uint32_t) count))
        {
            case BDD_TARGET_BOARD_DISK_OK:
                result = RES_OK;
                break;
            case BDD_TARGET_BOARD_DISK_OUT_OF_RANGE:
                result = RES_PARERR;
                break;
            default:
                result = RES_ERROR;
                break;
        }
    }
    return result;
}

/* Single logical drive 0 must be present and the backing image opened. */
static bool Diskio_IsDriveReady(BYTE pdrv)
{
    return (pdrv == 0) && BddTargetBoardDisk_IsReady();
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
    DRESULT result;
    if (!Diskio_IsDriveReady(pdrv))
    {
        result = RES_NOTRDY;
    }
    else
    {
        switch (BddTargetBoardDisk_Write(buff, (uint32_t) sector, (uint32_t) count))
        {
            case BDD_TARGET_BOARD_DISK_OK:
                result = RES_OK;
                break;
            case BDD_TARGET_BOARD_DISK_OUT_OF_RANGE:
                result = RES_PARERR;
                break;
            default:
                result = RES_ERROR;
                break;
        }
    }
    return result;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
    if (pdrv != 0)
    {
        return RES_PARERR;
    }
    switch (cmd)
    {
        case CTRL_SYNC:
            /* BddTargetBoardDisk_Write returns with the sectors written -
             * nothing is held back here to flush. */
            return RES_OK;
        case GET_SECTOR_COUNT:
            *(LBA_t*) buff = (LBA_t) BDD_TARGET_BOARD_DISK_SECTOR_COUNT;
            return RES_OK;
        case GET_SECTOR_SIZE:
            *(WORD*) buff = (WORD) BDD_TARGET_BOARD_DISK_SECTOR_SIZE;
            return RES_OK;
        case GET_BLOCK_SIZE:
            /* Erase-block size in sectors. The board's disk states no
             * erase granularity; 1 is the safe minimum. */
            *(DWORD*) buff = 1U;
            return RES_OK;
        default:
            return RES_PARERR;
    }
}
