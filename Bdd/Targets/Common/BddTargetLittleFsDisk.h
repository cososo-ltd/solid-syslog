#ifndef BDD_TARGET_LITTLE_FS_DISK_H
#define BDD_TARGET_LITTLE_FS_DISK_H

#include "SolidSyslogExternC.h"
#include "BddTargetBoardConfig.h"
#include "lfs.h"

#include <stdbool.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The LittleFS block device a board gives the target. There are two, and
     * the board's Board.cmake names the one it has the medium for:
     *
     * - BddTargetLittleFsDisk.c, over the board's sector disk
     *   (BddTargetBoardDisk.h), beside the ChaN-FatFs disk_* glue (diskio.c)
     *   and the FreeRTOS-Plus-FAT FF_Disk_t driver (BddTargetPlusFatDisk.c).
     *   A sector disk has no erase, so one is emulated by writing the erased
     *   value across the block. A program there can raise bits as well as
     *   clear them, which a real part cannot, so this device is a functional
     *   check and not a validation of flash behaviour.
     * - BddTargetLittleFsFlash.c, over a SPI NOR flash
     *   (BddTargetSpiNorFlash.h), where a block is one of the part's sectors
     *   and an erase is the part's own.
     *
     * Either way the filesystem is the size of the board's disk, as
     * BDD_TARGET_BOARD_DISK_SECTOR_SIZE and BDD_TARGET_BOARD_DISK_SECTOR_COUNT
     * in its BddTargetBoardConfig.h give it, in blocks of 4 KiB. The read and
     * program units are each device's own. */
    enum
    {
        BDD_TARGET_LITTLE_FS_DISK_BLOCK_SIZE = 4096,
        BDD_TARGET_LITTLE_FS_DISK_BLOCK_COUNT =
            (BDD_TARGET_BOARD_DISK_SECTOR_COUNT * BDD_TARGET_BOARD_DISK_SECTOR_SIZE) / 4096,
        BDD_TARGET_LITTLE_FS_DISK_CACHE_SIZE = BDD_TARGET_BOARD_DISK_SECTOR_SIZE,
        BDD_TARGET_LITTLE_FS_DISK_LOOKAHEAD_SIZE = 32
    };

    /* Fill in geometry, callbacks and the static buffers. The config is
     * file-scope storage here, so it outlives every mount. */
    const struct lfs_config* BddTargetLittleFsDisk_Config(void);

    /* Leaves the device with no filesystem on it, so the next mount formats
     * it. For a device that keeps its contents from one run of the target to
     * the next; call before anything is mounted. Returns false if the device
     * is cleared some other way, or the change fails. */
    bool BddTargetLittleFsDisk_Wipe(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDD_TARGET_LITTLE_FS_DISK_H */
