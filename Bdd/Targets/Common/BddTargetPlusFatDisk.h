#ifndef BDD_TARGET_PLUS_FAT_DISK_H
#define BDD_TARGET_PLUS_FAT_DISK_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* FreeRTOS-Plus-FAT FF_Disk_t media driver over the board's disk
     * (BddTargetBoardDisk.h). The Plus-FAT analogue of the ChaN-FatFs diskio.c
     * glue: same disk, same geometry - only the vtable shape differs (FF_Disk_t
     * block callbacks vs ChaN's global disk_*). Modelled on Plus-FAT's own
     * portable/common/ff_ramdisk.c reference driver, but persistent (the disk
     * survives the run) so it mounts an existing FAT and only partitions +
     * formats on a fresh / zero-filled disk - format-on-first-use, not
     * format-always.
     *
     * The single FF_Disk_t volume lives in this TU; the FF_IOManager cache is
     * allocated internally by Plus-FAT from the FreeRTOS heap (heap_4) - the one
     * place this target allocates for the filesystem, exactly the documented
     * vendor-allocates stance. Registered into ff_stdio's virtual FS at "/". */

    /* Ensure the board's disk, build the IO manager, mount (formatting on first
     * use), and register the volume at "/". Idempotent - repeated calls
     * short-circuit once mounted. Returns true once the volume is mounted. */
    bool BddTargetPlusFatDisk_Mount(void);

    /* Deregister, unmount, and release the IO manager (frees the Plus-FAT cache).
     * Safe to call when not mounted. */
    void BddTargetPlusFatDisk_Unmount(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDD_TARGET_PLUS_FAT_DISK_H */
