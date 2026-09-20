#ifndef BDDTARGETLITTLEFSMOUNT_H
#define BDDTARGETLITTLEFSMOUNT_H

#include "SolidSyslogExternC.h"

#include <stdbool.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* LittleFS implementation of the shared pipeline's FS-mount seam
     * (struct BddTargetFreeRtosPipelineConfig), beside the ChaN-FatFs and
     * FreeRTOS-Plus-FAT siblings. Owns the lfs_t and the mounted flag, and
     * supplies the per-file cache the adapter requires, so the pipeline stays
     * FS-vendor-agnostic. */

    /* Mount, formatting on first use when the image carries no filesystem.
     * Idempotent. Returns false on an unrecoverable failure so the caller can
     * leave the target on its original store. */
    bool BddTargetLittleFsMount_Mount(void);

    /* Unmount if mounted; no-op otherwise. */
    void BddTargetLittleFsMount_Unmount(void);

    /* Create / destroy the LittleFS SolidSyslogFile adapter (forward-declared
     * to keep lfs.h out of the seam's public surface). */
    struct SolidSyslogFile* BddTargetLittleFsMount_CreateFile(void);
    void BddTargetLittleFsMount_DestroyFile(struct SolidSyslogFile * file);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETLITTLEFSMOUNT_H */
