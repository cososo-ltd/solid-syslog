/* LittleFS implementation of the shared pipeline's FS-mount seam - see
 * BddTargetLittleFsMount.h. Mirrors BddTargetFatFsMount, which it replaces on
 * the CmsisLwip target. */

#include "BddTargetLittleFsMount.h"

#include "LfsSemihostingDisk.h"
#include "SolidSyslogLittleFsFile.h"

#include "lfs.h"

#include <stdio.h>

/* The lfs_t outlives every file opened from it, so it lives in .bss rather
 * than on the caller's stack. */
static lfs_t filesystem;
static bool mounted = false;

/* The adapter takes the per-file cache from the integrator, because only the
 * integrator knows the cache_size the filesystem was mounted with. Here that
 * integrator is this file, and the size comes from the same geometry the
 * block device declares. */
static unsigned char fileCache[LFS_SEMIHOSTING_CACHE_SIZE];

bool BddTargetLittleFsMount_Mount(void)
{
    if (mounted)
    {
        return true;
    }
    const struct lfs_config* config = LfsSemihostingDisk_Config();
    int result = lfs_mount(&filesystem, config);
    if (result == LFS_ERR_CORRUPT)
    {
        /* No filesystem on the image - lay one down and mount again. Behave
         * removes the image between scenarios, so this is the usual path
         * rather than the exception.
         *
         * Only LFS_ERR_CORRUPT triggers it, which is LittleFS's verdict that
         * there is nothing valid here. Formatting on any failure would let a
         * media error wipe an image that still holds records - the FatFs
         * sibling narrows to FR_NO_FILESYSTEM for the same reason. */
        result = lfs_format(&filesystem, config);
        if (result == LFS_ERR_OK)
        {
            result = lfs_mount(&filesystem, config);
        }
    }
    if (result != LFS_ERR_OK)
    {
        (void) printf("[solidsyslog] littlefs mount failed: lfs_error=%d\n", result);
        return false;
    }
    mounted = true;
    return true;
}

void BddTargetLittleFsMount_Unmount(void)
{
    if (mounted)
    {
        (void) lfs_unmount(&filesystem);
        mounted = false;
    }
}

struct SolidSyslogFile* BddTargetLittleFsMount_CreateFile(void)
{
    return SolidSyslogLittleFsFile_Create(&filesystem, fileCache, (uint32_t) sizeof(fileCache));
}

void BddTargetLittleFsMount_DestroyFile(struct SolidSyslogFile* file)
{
    /* Destroy closes, and the adapter's close is what commits the file's
     * metadata through lfs_file_close. */
    SolidSyslogLittleFsFile_Destroy(file);
}
