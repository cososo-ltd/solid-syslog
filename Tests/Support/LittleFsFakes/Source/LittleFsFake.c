#include "LittleFsFake.h"

#include <string.h>

static lfs_t Fake_Filesystem;
static struct lfs_config Fake_Config;

static int Fake_CloseCallCount;
static int Fake_OpenResult;
static int Fake_OpenCallCount;
static const char* Fake_LastOpenPath;
static int Fake_LastOpenFlags;
static const void* Fake_LastOpenBuffer;

void LittleFsFake_Reset(void)
{
    memset(&Fake_Filesystem, 0, sizeof(Fake_Filesystem));
    memset(&Fake_Config, 0, sizeof(Fake_Config));
    Fake_CloseCallCount = 0;
    Fake_OpenResult = LFS_ERR_OK;
    Fake_OpenCallCount = 0;
    Fake_LastOpenPath = NULL;
    Fake_LastOpenFlags = 0;
    Fake_LastOpenBuffer = NULL;
}

int LittleFsFake_CloseCallCount(void)
{
    return Fake_CloseCallCount;
}

int lfs_file_close(lfs_t* lfs, lfs_file_t* file)
{
    (void) lfs;
    (void) file;
    Fake_CloseCallCount++;
    return LFS_ERR_OK;
}

void LittleFsFake_SetOpenResult(int result)
{
    Fake_OpenResult = result;
}

int LittleFsFake_OpenCallCount(void)
{
    return Fake_OpenCallCount;
}

const char* LittleFsFake_LastOpenPath(void)
{
    return Fake_LastOpenPath;
}

int LittleFsFake_LastOpenFlags(void)
{
    return Fake_LastOpenFlags;
}

const void* LittleFsFake_LastOpenBuffer(void)
{
    return Fake_LastOpenBuffer;
}

int lfs_file_opencfg(lfs_t* lfs, lfs_file_t* file, const char* path, int flags, const struct lfs_file_config* config)
{
    (void) lfs;
    (void) file;
    Fake_OpenCallCount++;
    Fake_LastOpenPath = path;
    Fake_LastOpenFlags = flags;
    Fake_LastOpenBuffer = (config != NULL) ? config->buffer : NULL;
    return Fake_OpenResult;
}

lfs_t* LittleFsFake_MountedWithCacheSize(lfs_size_t cacheSize)
{
    Fake_Config.cache_size = cacheSize;
    Fake_Filesystem.cfg = &Fake_Config;
    return &Fake_Filesystem;
}
