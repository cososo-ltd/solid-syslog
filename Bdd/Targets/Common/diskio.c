/* ChaN-FatFs disk_* glue over the shared semihosting media (SemihostingDisk).
 *
 * Maps FatFs's caller-facing disk_* contract onto the host-backed flat disk
 * image that both FreeRTOS BDD targets share. The semihosting trap, the 8 MiB
 * FAT16 geometry, and the open-or-create-sparse logic live in
 * Bdd/Targets/Common/SemihostingDisk so the FreeRTOS-Plus-FAT FF_Disk_t driver
 * (FFSemihostingDisk.c) exercises byte-for-byte the same image (S29.05).
 *
 * Used by the lwIP BDD target (FatFs-backed store); the FreeRTOS-Plus-TCP
 * target uses Plus-FAT instead. Single logical drive (pdrv 0); no exFAT, no
 * LFN. */

/* ff.h before diskio.h: diskio.h declares disk_* in terms of BYTE / UINT /
 * LBA_t / DWORD / WORD which are typedef'd in ff.h's integer headers. */
#include "ff.h"
#include "diskio.h"

#include "SemihostingDisk.h"

#include <stdbool.h>

static bool Diskio_IsDriveReady(BYTE pdrv);

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0)
    {
        return STA_NOINIT;
    }
    return SemihostingDisk_EnsureReady() ? 0 : STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0)
    {
        return STA_NOINIT;
    }
    return SemihostingDisk_IsReady() ? 0 : STA_NOINIT;
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
        switch (SemihostingDisk_Read(buff, (uint32_t) sector, (uint32_t) count))
        {
            case SEMIHOSTING_DISK_OK:
                result = RES_OK;
                break;
            case SEMIHOSTING_DISK_OUT_OF_RANGE:
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
    return (pdrv == 0) && SemihostingDisk_IsReady();
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
        switch (SemihostingDisk_Write(buff, (uint32_t) sector, (uint32_t) count))
        {
            case SEMIHOSTING_DISK_OK:
                result = RES_OK;
                break;
            case SEMIHOSTING_DISK_OUT_OF_RANGE:
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
            /* QEMU semihosting writes are synchronous against the host
             * file — no kernel-level dirty pages we need to flush. */
            return RES_OK;
        case GET_SECTOR_COUNT:
            *(LBA_t*) buff = (LBA_t) SEMIHOSTING_DISK_SECTOR_COUNT;
            return RES_OK;
        case GET_SECTOR_SIZE:
            *(WORD*) buff = (WORD) SEMIHOSTING_DISK_SECTOR_SIZE;
            return RES_OK;
        case GET_BLOCK_SIZE:
            /* Erase-block size in sectors. The semihosting flat file
             * has no native erase granularity; 1 is the safe minimum. */
            *(DWORD*) buff = 1U;
            return RES_OK;
        default:
            return RES_PARERR;
    }
}
