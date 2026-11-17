#ifndef BDD_TARGET_PLUS_FAT_DISK_H
#define BDD_TARGET_PLUS_FAT_DISK_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* FreeRTOS-Plus-FAT FF_Disk_t media driver over the board's disk
     * (BddTargetBoardDisk.h). Modelled on Plus-FAT's own
     * portable/common/ff_ramdisk.c reference driver, but the disk outlasts the
     * run, so it mounts an existing FAT and partitions and formats only a
     * disk that will not mount.
     *
     * The single FF_Disk_t volume lives in this TU. Plus-FAT allocates the
     * FF_IOManager cache itself from the FreeRTOS heap (heap_4), which is the
     * one allocation this target makes for the filesystem. The volume is
     * registered into ff_stdio's virtual FS at "/". */

    /* Ensure the board's disk, build the IO manager, mount (formatting on first
     * use), and register the volume at "/". Idempotent - repeated calls
     * short-circuit once mounted. Returns true once the volume is mounted. */
    bool BddTargetPlusFatDisk_Mount(void);

    /* Deregister, unmount, and release the IO manager (frees the Plus-FAT cache).
     * Safe to call when not mounted. */
    void BddTargetPlusFatDisk_Unmount(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDD_TARGET_PLUS_FAT_DISK_H */
