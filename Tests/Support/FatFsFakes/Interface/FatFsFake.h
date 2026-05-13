#ifndef FATFSFAKE_H
#define FATFSFAKE_H

#include "ExternC.h"
#include "ff.h"

EXTERN_C_BEGIN

    void FatFsFake_Reset(void);

    /* f_open */
    void          FatFsFake_SetOpenResult(FRESULT result);
    int           FatFsFake_OpenCallCount(void);
    const char*   FatFsFake_LastOpenPath(void);
    unsigned char FatFsFake_LastOpenMode(void);

    /* f_close */
    int FatFsFake_CloseCallCount(void);

    /* f_lseek */
    int           FatFsFake_LseekCallCount(void);
    unsigned long FatFsFake_LastLseekOffset(void);

    /* f_truncate */
    int FatFsFake_TruncateCallCount(void);

    /* f_read */
    void         FatFsFake_SetReadResult(FRESULT result);
    void         FatFsFake_SetReadBytesReturned(unsigned int bytes);
    void         FatFsFake_SetReadSource(const void* bytes, unsigned int count);
    int          FatFsFake_ReadCallCount(void);
    unsigned int FatFsFake_LastReadCount(void);

    /* f_write */
    void         FatFsFake_SetWriteResult(FRESULT result);
    void         FatFsFake_SetWriteBytesReturned(unsigned int bytes);
    int          FatFsFake_WriteCallCount(void);
    const void*  FatFsFake_LastWriteBytes(void);
    unsigned int FatFsFake_LastWriteCount(void);

    /* f_sync */
    void FatFsFake_SetSyncResult(FRESULT result);
    int  FatFsFake_SyncCallCount(void);

    /* f_stat */
    void        FatFsFake_SetStatResult(FRESULT result);
    int         FatFsFake_StatCallCount(void);
    const char* FatFsFake_LastStatPath(void);

    /* f_unlink */
    void        FatFsFake_SetUnlinkResult(FRESULT result);
    int         FatFsFake_UnlinkCallCount(void);
    const char* FatFsFake_LastUnlinkPath(void);

    /* file size — writes obj.objsize on the last-opened FIL, so f_size(fp)
     * (a macro that dereferences fp->obj.objsize) returns the programmed
     * value. */
    void FatFsFake_SetFileSize(unsigned long size);

EXTERN_C_END

#endif /* FATFSFAKE_H */
