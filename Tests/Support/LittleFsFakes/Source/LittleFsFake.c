#include "LittleFsFake.h"

#include <string.h>

static lfs_t Fake_Filesystem;
static struct lfs_config Fake_Config;

void LittleFsFake_Reset(void)
{
    memset(&Fake_Filesystem, 0, sizeof(Fake_Filesystem));
    memset(&Fake_Config, 0, sizeof(Fake_Config));
}

lfs_t* LittleFsFake_MountedWithCacheSize(lfs_size_t cacheSize)
{
    Fake_Config.cache_size = cacheSize;
    Fake_Filesystem.cfg = &Fake_Config;
    return &Fake_Filesystem;
}
