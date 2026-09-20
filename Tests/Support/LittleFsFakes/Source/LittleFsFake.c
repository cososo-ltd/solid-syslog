#include "LittleFsFake.h"

#include <stdbool.h>
#include <string.h>

static lfs_t Fake_Filesystem;
static struct lfs_config Fake_Config;

static int Fake_CloseCallCount;
static lfs_ssize_t Fake_WriteAccepted;
static bool Fake_WriteAcceptedSet;
static unsigned char Fake_WriteCapture[64];
static lfs_size_t Fake_LastWriteCount;
static int Fake_SyncResult;
static int Fake_SyncCallCount;
static const unsigned char* Fake_ReadSource;
static lfs_size_t Fake_ReadAvailable;
static int Fake_ReadCallCount;
static lfs_size_t Fake_LastReadCount;
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
    Fake_WriteAccepted = 0;
    Fake_WriteAcceptedSet = false;
    memset(Fake_WriteCapture, 0, sizeof(Fake_WriteCapture));
    Fake_LastWriteCount = 0;
    Fake_SyncResult = LFS_ERR_OK;
    Fake_SyncCallCount = 0;
    Fake_ReadSource = NULL;
    Fake_ReadAvailable = 0;
    Fake_ReadCallCount = 0;
    Fake_LastReadCount = 0;
    Fake_OpenResult = LFS_ERR_OK;
    Fake_OpenCallCount = 0;
    Fake_LastOpenPath = NULL;
    Fake_LastOpenFlags = 0;
    Fake_LastOpenBuffer = NULL;
}

void LittleFsFake_SetReadSource(const void* bytes, lfs_size_t count)
{
    Fake_ReadSource = (const unsigned char*) bytes;
    Fake_ReadAvailable = count;
}

int LittleFsFake_ReadCallCount(void)
{
    return Fake_ReadCallCount;
}

lfs_size_t LittleFsFake_LastReadCount(void)
{
    return Fake_LastReadCount;
}

lfs_ssize_t lfs_file_read(lfs_t* lfs, lfs_file_t* file, void* buffer, lfs_size_t size)
{
    (void) lfs;
    (void) file;
    lfs_size_t served = (size < Fake_ReadAvailable) ? size : Fake_ReadAvailable;
    Fake_ReadCallCount++;
    Fake_LastReadCount = size;
    if ((Fake_ReadSource != NULL) && (served > 0U))
    {
        memcpy(buffer, Fake_ReadSource, served);
    }
    return (lfs_ssize_t) served;
}

void LittleFsFake_SetWriteBytesAccepted(lfs_ssize_t bytes)
{
    Fake_WriteAccepted = bytes;
    Fake_WriteAcceptedSet = true;
}

const void* LittleFsFake_LastWriteBytes(void)
{
    return Fake_WriteCapture;
}

lfs_size_t LittleFsFake_LastWriteCount(void)
{
    return Fake_LastWriteCount;
}

void LittleFsFake_SetSyncResult(int result)
{
    Fake_SyncResult = result;
}

int LittleFsFake_SyncCallCount(void)
{
    return Fake_SyncCallCount;
}

lfs_ssize_t lfs_file_write(lfs_t* lfs, lfs_file_t* file, const void* buffer, lfs_size_t size)
{
    (void) lfs;
    (void) file;
    Fake_LastWriteCount = size;
    lfs_size_t captured = (size < sizeof(Fake_WriteCapture)) ? size : (lfs_size_t) sizeof(Fake_WriteCapture);
    memcpy(Fake_WriteCapture, buffer, captured);
    /* Accepts the whole write unless a test says otherwise. */
    return Fake_WriteAcceptedSet ? Fake_WriteAccepted : (lfs_ssize_t) size;
}

int lfs_file_sync(lfs_t* lfs, lfs_file_t* file)
{
    (void) lfs;
    (void) file;
    Fake_SyncCallCount++;
    return Fake_SyncResult;
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
