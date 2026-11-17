#ifndef BDDTARGETPLUSFATMOUNT_H
#define BDDTARGETPLUSFATMOUNT_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* FreeRTOS-Plus-FAT implementation of the shared pipeline's FS-mount seam
     * (struct BddTargetFreeRtosPipelineConfig). Mount and unmount delegate to
     * the FF_Disk_t media driver (BddTargetPlusFatDisk), and the
     * SolidSyslogFile adapter is SolidSyslogPlusFatFile. */

    /* Mount the Plus-FAT volume, formatting on first use. Idempotent; false on
     * an unrecoverable mount/format failure so the caller can leave the target
     * on its original store. */
    bool BddTargetPlusFatMount_Mount(void);

    /* Unmount the Plus-FAT volume if mounted; no-op otherwise. */
    void BddTargetPlusFatMount_Unmount(void);

    /* Calls BddTargetBoardDisk_Wipe. */
    bool BddTargetPlusFatMount_Wipe(void);

    /* Create / destroy the Plus-FAT SolidSyslogFile adapter (forward-declared to
     * keep ff_stdio.h / SolidSyslogPlusFatFile.h out of the seam's public
     * surface). */
    struct SolidSyslogFile* BddTargetPlusFatMount_CreateFile(void);
    void BddTargetPlusFatMount_DestroyFile(struct SolidSyslogFile * file);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETPLUSFATMOUNT_H */
