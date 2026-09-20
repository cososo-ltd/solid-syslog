#ifndef LFSSEMIHOSTINGDISK_H
#define LFSSEMIHOSTINGDISK_H

#include "SolidSyslogExternC.h"
#include "lfs.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* LittleFS block device over the shared host-backed image - the third
     * driver on SemihostingDisk.c, beside the ChaN-FatFs disk_* glue
     * (diskio.c) and the FreeRTOS-Plus-FAT FF_Disk_t one
     * (FFSemihostingDisk.c). One image, one geometry, one semihosting trap.
     *
     * The image is a flat disk rather than flash, so it has no erase. Erase is
     * emulated by writing the erased value across the block, which gives
     * LittleFS the 0xFF it expects to find and to program down from. That makes
     * the emulation more permissive than a real part - a program here can raise
     * bits as well as clear them - which is the right trade for a functional
     * lane and the reason the platform page does not claim flash validation. */

    /* Geometry over the shared 512 B x 16384 sector image: 4 KiB blocks, so a
     * block is eight sectors and the whole 8 MiB is 2048 of them. */
    enum
    {
        LFS_SEMIHOSTING_READ_SIZE = 512,
        LFS_SEMIHOSTING_PROG_SIZE = 512,
        LFS_SEMIHOSTING_BLOCK_SIZE = 4096,
        LFS_SEMIHOSTING_BLOCK_COUNT = 2048,
        LFS_SEMIHOSTING_CACHE_SIZE = 512,
        LFS_SEMIHOSTING_LOOKAHEAD_SIZE = 32
    };

    /* Fill in geometry, callbacks and the static buffers. The config is
     * file-scope storage here, so it outlives every mount. */
    const struct lfs_config* LfsSemihostingDisk_Config(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* LFSSEMIHOSTINGDISK_H */
