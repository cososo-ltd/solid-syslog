#ifndef FFSEMIHOSTINGDISK_H
#define FFSEMIHOSTINGDISK_H

#include "ExternC.h"

#include <stdbool.h>

EXTERN_C_BEGIN

    /* FreeRTOS-Plus-FAT FF_Disk_t media driver over the shared semihosting flat
     * disk (SemihostingDisk) for the QEMU mps2-an385 BDD target. The Plus-FAT
     * analogue of the ChaN-FatFs diskio.c glue: same host image, same 8 MiB
     * FAT16 geometry, same BKPT 0xAB trap — only the vtable shape differs
     * (FF_Disk_t block callbacks vs ChaN's global disk_*). Modelled on Plus-FAT's
     * own portable/common/ff_ramdisk.c reference driver, but persistent (the host
     * image survives the run) so it mounts an existing FAT and only partitions +
     * formats on a fresh / zero-filled image — format-on-first-use, not
     * format-always.
     *
     * The single FF_Disk_t volume lives in this TU; the FF_IOManager cache is
     * allocated internally by Plus-FAT from the FreeRTOS heap (heap_4) — the one
     * place this target allocates for the filesystem, exactly the documented
     * vendor-allocates stance. Registered into ff_stdio's virtual FS at "/".
     * Introduced in SolidSyslog S29.05. */

    /* Ensure the host image, build the IO manager, mount (formatting on first
     * use), and register the volume at "/". Idempotent — repeated calls
     * short-circuit once mounted. Returns true once the volume is mounted. */
    bool FFSemihostingDisk_Mount(void);

    /* Deregister, unmount, and release the IO manager (frees the Plus-FAT cache).
     * Safe to call when not mounted. */
    void FFSemihostingDisk_Unmount(void);

EXTERN_C_END

#endif /* FFSEMIHOSTINGDISK_H */
