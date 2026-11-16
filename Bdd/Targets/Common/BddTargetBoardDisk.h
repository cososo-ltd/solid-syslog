#ifndef BDD_TARGET_BOARD_DISK_H
#define BDD_TARGET_BOARD_DISK_H

#include "SolidSyslogExternC.h"

#include "BddTargetBoardConfig.h"

#include <stdbool.h>
#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The board's sector disk. Part of the board seam - see BddTargetBoard.h -
     * and defined in the board's directory.
     *
     * Each filesystem a target may run on sectors has its media glue in this
     * directory - the ChaN-FatFs disk_* functions (diskio.c), the
     * FreeRTOS-Plus-FAT FF_Disk_t driver (BddTargetPlusFatDisk.c) and the
     * LittleFS block device for a board with no flash (BddTargetLittleFsDisk.c)
     * - and all three sit on the first four calls below, so a board supplies
     * its sectors once.
     *
     * The geometry is the board's, as BDD_TARGET_BOARD_DISK_SECTOR_SIZE and
     * BDD_TARGET_BOARD_DISK_SECTOR_COUNT in its BddTargetBoardConfig.h. */

    enum BddTargetBoardDiskResult
    {
        BDD_TARGET_BOARD_DISK_OK = 0,
        BDD_TARGET_BOARD_DISK_OUT_OF_RANGE,
        BDD_TARGET_BOARD_DISK_IO_ERROR,
    };

    /* Makes the disk usable. Idempotent - a repeated call returns at once.
     * Returns true once it is. A disk the board had to create reads as zeros,
     * so the filesystem above finds nothing and formats on first use. */
    bool BddTargetBoardDisk_EnsureReady(void);

    /* True once EnsureReady has succeeded. */
    bool BddTargetBoardDisk_IsReady(void);

    /* Read / write `count` consecutive sectors starting at LBA `sector`.
     * Both validate the range against the disk geometry before touching the
     * media. The caller is responsible for checking readiness first. */
    enum BddTargetBoardDiskResult BddTargetBoardDisk_Read(void* buffer, uint32_t sector, uint32_t count);
    enum BddTargetBoardDiskResult BddTargetBoardDisk_Write(const void* buffer, uint32_t sector, uint32_t count);

    /* Leaves the disk with no filesystem on it, so that the next mount
     * formats it. It is for a disk that keeps what it holds from one run of
     * the target to the next, and is asked for before anything is mounted.
     * False from a board whose disk is put back to nothing some other way,
     * and from one whose disk will not take the write. */
    bool BddTargetBoardDisk_Wipe(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDD_TARGET_BOARD_DISK_H */
