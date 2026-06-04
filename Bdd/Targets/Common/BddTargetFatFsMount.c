/* ChaN-FatFs implementation of the shared pipeline's FS-mount seam — see
 * BddTargetFatFsMount.h. Extracted from BddTargetFreeRtosPipeline.c in
 * SolidSyslog S29.05 when the two FreeRTOS targets first diverged on the
 * filesystem (FatFs on lwIP, FreeRTOS-Plus-FAT on Plus-TCP). The logic here is
 * the former in-pipeline EnsureFatFsMounted / f_unmount / SolidSyslogFatFsFile_*
 * path, lifted verbatim. */

#include "BddTargetFatFsMount.h"

#include "SolidSyslogFatFsFile.h"

#include "ff.h" /* f_mount / f_mkfs — eager mount-or-format on the `set store file` rebuild trigger. */

#include <stdio.h>

/* FATFS object lives in .bss because f_mount stores its address inside the FatFs
 * volume registry — the object must outlive every f_open / f_stat / f_unlink.
 * One per volume (FF_VOLUMES = 1). */
static FATFS fatfs;
static bool fatfsMounted = false;

bool BddTargetFatFsMount_Mount(void)
{
    if (fatfsMounted)
    {
        return true;
    }
    FRESULT res = f_mount(&fatfs, "", 1); /* opt=1 → mount immediately, surface FR_NO_FILESYSTEM here */
    if (res == FR_NO_FILESYSTEM)
    {
        /* Fresh disk image — lay down a FAT and re-mount. FM_FAT keeps the
         * formatter on FAT12/16; at the shared 8 MiB geometry auto cluster
         * sizing clears the ~4085-cluster boundary, so this lands FAT16 (the
         * geometry the FreeRTOS-Plus-FAT formatter needs on the sibling
         * target). The work buffer is sized to FF_MAX_SS (512 B), the minimum
         * f_mkfs accepts on a FAT12/16 volume. */
        static BYTE workBuffer[FF_MAX_SS];
        const MKFS_PARM opts = {.fmt = FM_FAT | FM_SFD, .n_fat = 1, .align = 1, .n_root = 0, .au_size = 0};
        res = f_mkfs("", &opts, workBuffer, sizeof(workBuffer));
        if (res == FR_OK)
        {
            res = f_mount(&fatfs, "", 1);
        }
    }
    if (res != FR_OK)
    {
        (void) printf("[solidsyslog] fatfs mount failed: FRESULT=%d\n", (int) res);
        return false;
    }
    fatfsMounted = true;
    return true;
}

void BddTargetFatFsMount_Unmount(void)
{
    if (fatfsMounted)
    {
        (void) f_unmount("");
        fatfsMounted = false;
    }
}

struct SolidSyslogFile* BddTargetFatFsMount_CreateFile(void)
{
    return SolidSyslogFatFsFile_Create();
}

void BddTargetFatFsMount_DestroyFile(struct SolidSyslogFile* file)
{
    /* FatFsFile_Destroy → Close → f_close flushes the underlying FIL's dir entry. */
    SolidSyslogFatFsFile_Destroy(file);
}
