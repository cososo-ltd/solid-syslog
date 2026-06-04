#ifndef SEMIHOSTINGDISK_H
#define SEMIHOSTINGDISK_H

#include "ExternC.h"

#include <stdbool.h>
#include <stdint.h>

EXTERN_C_BEGIN

    /* Host-backed flat-disk media access over ARM semihosting (BKPT 0xAB),
     * shared by both filesystem media drivers on the QEMU mps2-an385 BDD
     * targets: the ChaN-FatFs disk_* glue (diskio.c) and the FreeRTOS-Plus-FAT
     * FF_Disk_t driver (FFSemihostingDisk.c). One image, one geometry, one
     * semihosting trap — so a FatFs run and a Plus-FAT run exercise byte-for-
     * byte the same host file (solidsyslog-disk.img in QEMU's working dir).
     *
     * Behave's after_scenario removes the image so each scenario starts with
     * no filesystem; EnsureReady creates a fresh 8 MiB sparse file when it sees
     * the open fail or the file is too small, the filesystem's mount then sees
     * a zero-filled image and the integrator falls through to format-on-first-
     * use.
     *
     * Single logical drive; 16384 sectors x 512 B = 8 MiB — large enough for
     * the store-and-forward / capacity / power-cycle scenarios that exercise
     * multi-block files, and clear of the ~4085-cluster FAT12/16 boundary so
     * both formatters land FAT16. */

    enum
    {
        SEMIHOSTING_DISK_SECTOR_SIZE = 512,
        SEMIHOSTING_DISK_SECTOR_COUNT = 16384,
    };

    enum SemihostingDiskResult
    {
        SEMIHOSTING_DISK_OK = 0,
        SEMIHOSTING_DISK_OUT_OF_RANGE,
        SEMIHOSTING_DISK_IO_ERROR,
    };

    /* Open the host image, creating + sparse-extending a fresh zero-filled 8 MiB
     * file on first use. Idempotent — repeated calls short-circuit on the cached
     * handle. Returns true once a usable handle is held. */
    bool SemihostingDisk_EnsureReady(void);

    /* True once EnsureReady has acquired a handle. */
    bool SemihostingDisk_IsReady(void);

    /* Read / write `count` consecutive 512 B sectors starting at LBA `sector`.
     * Both validate the range against the disk geometry before touching the
     * image. The caller is responsible for checking readiness first. */
    enum SemihostingDiskResult SemihostingDisk_Read(void* buffer, uint32_t sector, uint32_t count);
    enum SemihostingDiskResult SemihostingDisk_Write(const void* buffer, uint32_t sector, uint32_t count);

EXTERN_C_END

#endif /* SEMIHOSTINGDISK_H */
