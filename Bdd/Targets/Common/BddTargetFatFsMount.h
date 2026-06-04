#ifndef BDDTARGETFATFSMOUNT_H
#define BDDTARGETFATFSMOUNT_H

#include "ExternC.h"

#include <stdbool.h>

EXTERN_C_BEGIN

    /* ChaN-FatFs implementation of the shared pipeline's FS-mount seam
     * (struct BddTargetFreeRtosPipelineConfig). Owns the single-volume FATFS
     * registry object + mounted flag and wraps the ff_* lifecycle so the
     * pipeline stays FS-vendor-agnostic. Wired by the lwIP target's main.c;
     * the FreeRTOS-Plus-TCP target uses the FreeRTOS-Plus-FAT sibling
     * (BddTargetPlusFatMount). */

    /* Mount volume 0, formatting on first use if the disk image has no FAT
     * yet. Idempotent — repeated calls short-circuit on the mounted flag.
     * Returns false on an unrecoverable mount/format failure so the caller
     * can leave the target on its original store. */
    bool BddTargetFatFsMount_Mount(void);

    /* Unmount volume 0 if mounted; no-op otherwise. */
    void BddTargetFatFsMount_Unmount(void);

    /* Create / destroy the ChaN-FatFs SolidSyslogFile adapter (forward-declared
     * to keep ff.h / SolidSyslogFatFsFile.h out of the seam's public surface). */
    struct SolidSyslogFile* BddTargetFatFsMount_CreateFile(void);
    void BddTargetFatFsMount_DestroyFile(struct SolidSyslogFile * file);

EXTERN_C_END

#endif /* BDDTARGETFATFSMOUNT_H */
