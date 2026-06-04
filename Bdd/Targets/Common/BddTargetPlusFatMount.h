#ifndef BDDTARGETPLUSFATMOUNT_H
#define BDDTARGETPLUSFATMOUNT_H

#include "ExternC.h"

#include <stdbool.h>

EXTERN_C_BEGIN

    /* FreeRTOS-Plus-FAT implementation of the shared pipeline's FS-mount seam
     * (struct BddTargetFreeRtosPipelineConfig). The Plus-FAT sibling of
     * BddTargetFatFsMount: mount/unmount delegate to the FF_Disk_t semihosting
     * media driver (FFSemihostingDisk); the SolidSyslogFile adapter is
     * SolidSyslogPlusFatFile. Wired by the FreeRTOS-Plus-TCP target's main.c;
     * the lwIP target uses the ChaN-FatFs sibling. Introduced in SolidSyslog
     * S29.05. */

    /* Mount the Plus-FAT volume, formatting on first use. Idempotent; false on
     * an unrecoverable mount/format failure so the caller can leave the target
     * on its original store. */
    bool BddTargetPlusFatMount_Mount(void);

    /* Unmount the Plus-FAT volume if mounted; no-op otherwise. */
    void BddTargetPlusFatMount_Unmount(void);

    /* Create / destroy the Plus-FAT SolidSyslogFile adapter (forward-declared to
     * keep ff_stdio.h / SolidSyslogPlusFatFile.h out of the seam's public
     * surface). */
    struct SolidSyslogFile* BddTargetPlusFatMount_CreateFile(void);
    void BddTargetPlusFatMount_DestroyFile(struct SolidSyslogFile* file);

EXTERN_C_END

#endif /* BDDTARGETPLUSFATMOUNT_H */
