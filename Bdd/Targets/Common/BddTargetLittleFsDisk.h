#ifndef BDD_TARGET_LITTLE_FS_DISK_H
#define BDD_TARGET_LITTLE_FS_DISK_H

#include "SolidSyslogExternC.h"
#include "BddTargetBoardConfig.h"
#include "lfs.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* LittleFS block device over the board's disk (BddTargetBoardDisk.h) - the
     * third driver on it, beside the ChaN-FatFs disk_* glue (diskio.c) and the
     * FreeRTOS-Plus-FAT FF_Disk_t one (BddTargetPlusFatDisk.c). One disk, one
     * geometry.
     *
     * The seam presents a sector disk rather than flash, so it has no erase. Erase is
     * emulated by writing the erased value across the block, which gives
     * LittleFS the 0xFF it expects to find and to program down from. That makes
     * the emulation more permissive than a real part - a program here can raise
     * bits as well as clear them - which is the right trade for a functional
     * lane and the reason the platform page does not claim flash validation. */

    /* Geometry over the board's sectors: the read and program units are one
     * sector, and a block is 4 KiB of them. */
    enum
    {
        BDD_TARGET_LITTLE_FS_DISK_READ_SIZE = BDD_TARGET_BOARD_DISK_SECTOR_SIZE,
        BDD_TARGET_LITTLE_FS_DISK_PROG_SIZE = BDD_TARGET_BOARD_DISK_SECTOR_SIZE,
        BDD_TARGET_LITTLE_FS_DISK_BLOCK_SIZE = 4096,
        BDD_TARGET_LITTLE_FS_DISK_BLOCK_COUNT =
            (BDD_TARGET_BOARD_DISK_SECTOR_COUNT * BDD_TARGET_BOARD_DISK_SECTOR_SIZE) / 4096,
        BDD_TARGET_LITTLE_FS_DISK_CACHE_SIZE = BDD_TARGET_BOARD_DISK_SECTOR_SIZE,
        BDD_TARGET_LITTLE_FS_DISK_LOOKAHEAD_SIZE = 32
    };

    /* Fill in geometry, callbacks and the static buffers. The config is
     * file-scope storage here, so it outlives every mount. */
    const struct lfs_config* BddTargetLittleFsDisk_Config(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDD_TARGET_LITTLE_FS_DISK_H */
