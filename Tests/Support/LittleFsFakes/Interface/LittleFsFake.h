#ifndef LITTLEFSFAKE_H
#define LITTLEFSFAKE_H

#include "SolidSyslogExternC.h"
#include "lfs.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void LittleFsFake_Reset(void);

    /* A mounted filesystem whose cfg->cache_size is what the adapter checks a
     * caller's file buffer against. The storage lives in the fake. */
    lfs_t* LittleFsFake_MountedWithCacheSize(lfs_size_t cacheSize);

    int LittleFsFake_CloseCallCount(void);

    /* Bytes lfs_file_read hands back, and how many of them. A read asking for
       more than this returns the short count, which is how the adapter's
       short-read path is driven. */
    void LittleFsFake_SetReadSource(const void* bytes, lfs_size_t count);
    int LittleFsFake_ReadCallCount(void);
    lfs_size_t LittleFsFake_LastReadCount(void);

    void LittleFsFake_SetWriteBytesAccepted(lfs_ssize_t bytes);
    const void* LittleFsFake_LastWriteBytes(void);
    lfs_size_t LittleFsFake_LastWriteCount(void);

    void LittleFsFake_SetSyncResult(int result);
    int LittleFsFake_SyncCallCount(void);

    lfs_soff_t LittleFsFake_LastSeekOffset(void);
    int LittleFsFake_LastSeekWhence(void);

    void LittleFsFake_SetFileSize(lfs_soff_t size);
    void LittleFsFake_SetFileSizeError(int error);

    lfs_off_t LittleFsFake_LastTruncateSize(void);

    void LittleFsFake_SetStatResult(int result);
    const char* LittleFsFake_LastStatPath(void);

    void LittleFsFake_SetRemoveResult(int result);
    const char* LittleFsFake_LastRemovePath(void);

    void LittleFsFake_SetOpenResult(int result);
    int LittleFsFake_OpenCallCount(void);
    const char* LittleFsFake_LastOpenPath(void);
    int LittleFsFake_LastOpenFlags(void);
    const void* LittleFsFake_LastOpenBuffer(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* LITTLEFSFAKE_H */
