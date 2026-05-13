#include "FatFsFake.h"

#include <stdbool.h>
#include <stddef.h>

#include "ff.h"

/* f_open state */
static int         openCallCount;
static const char* lastOpenPath;
static BYTE        lastOpenMode;
static FRESULT     openResult;
static FIL*        lastOpenedFp;

/* f_close state */
static int closeCallCount;

/* f_lseek state */
static int     lseekCallCount;
static FSIZE_t lastLseekOffset;

/* f_truncate state */
static int truncateCallCount;

/* f_read state */
static int         readCallCount;
static const void* lastReadBuf;
static UINT        lastReadCount;
static FRESULT     readResult;
static UINT        readBytesReturned;
static bool        readBytesReturnedOverridden;

/* f_write state */
static int         writeCallCount;
static const void* lastWriteBuf;
static UINT        lastWriteCount;
static FRESULT     writeResult;
static UINT        writeBytesReturned;
static bool        writeBytesReturnedOverridden;

/* f_stat state */
static int         statCallCount;
static const char* lastStatPath;
static FRESULT     statResult;

/* f_unlink state */
static int         unlinkCallCount;
static const char* lastUnlinkPath;
static FRESULT     unlinkResult;

void FatFsFake_Reset(void)
{
    openCallCount                = 0;
    lastOpenPath                 = NULL;
    lastOpenMode                 = 0;
    openResult                   = FR_OK;
    lastOpenedFp                 = NULL;
    closeCallCount               = 0;
    lseekCallCount               = 0;
    lastLseekOffset              = 0;
    truncateCallCount            = 0;
    readCallCount                = 0;
    lastReadBuf                  = NULL;
    lastReadCount                = 0;
    readResult                   = FR_OK;
    readBytesReturned            = 0;
    readBytesReturnedOverridden  = false;
    writeCallCount               = 0;
    lastWriteBuf                 = NULL;
    lastWriteCount               = 0;
    writeResult                  = FR_OK;
    writeBytesReturned           = 0;
    writeBytesReturnedOverridden = false;
    statCallCount                = 0;
    lastStatPath                 = NULL;
    statResult                   = FR_OK;
    unlinkCallCount              = 0;
    lastUnlinkPath               = NULL;
    unlinkResult                 = FR_OK;
}

void FatFsFake_SetOpenResult(FRESULT result)
{
    openResult = result;
}

int FatFsFake_OpenCallCount(void)
{
    return openCallCount;
}

const char* FatFsFake_LastOpenPath(void)
{
    return lastOpenPath;
}

unsigned char FatFsFake_LastOpenMode(void)
{
    return lastOpenMode;
}

FRESULT f_open(FIL* fp, const TCHAR* path, BYTE mode)
{
    openCallCount++;
    lastOpenPath = path;
    lastOpenMode = mode;
    lastOpenedFp = fp;
    return openResult;
}

int FatFsFake_CloseCallCount(void)
{
    return closeCallCount;
}

FRESULT f_close(FIL* fp)
{
    (void) fp;
    closeCallCount++;
    return FR_OK;
}

int FatFsFake_LseekCallCount(void)
{
    return lseekCallCount;
}

unsigned long FatFsFake_LastLseekOffset(void)
{
    return (unsigned long) lastLseekOffset;
}

FRESULT f_lseek(FIL* fp, FSIZE_t ofs)
{
    (void) fp;
    lseekCallCount++;
    lastLseekOffset = ofs;
    return FR_OK;
}

int FatFsFake_TruncateCallCount(void)
{
    return truncateCallCount;
}

FRESULT f_truncate(FIL* fp)
{
    (void) fp;
    truncateCallCount++;
    return FR_OK;
}

void FatFsFake_SetFileSize(unsigned long size)
{
    if (lastOpenedFp != NULL)
    {
        lastOpenedFp->obj.objsize = (FSIZE_t) size;
    }
}

void FatFsFake_SetReadResult(FRESULT result)
{
    readResult = result;
}

void FatFsFake_SetReadBytesReturned(unsigned int bytes)
{
    readBytesReturned           = bytes;
    readBytesReturnedOverridden = true;
}

int FatFsFake_ReadCallCount(void)
{
    return readCallCount;
}

const void* FatFsFake_LastReadBuf(void)
{
    return lastReadBuf;
}

unsigned int FatFsFake_LastReadCount(void)
{
    return lastReadCount;
}

FRESULT f_read(FIL* fp, void* buff, UINT btr, UINT* br)
{
    (void) fp;
    readCallCount++;
    lastReadBuf   = buff;
    lastReadCount = btr;
    *br           = readBytesReturnedOverridden ? readBytesReturned : btr;
    return readResult;
}

void FatFsFake_SetWriteResult(FRESULT result)
{
    writeResult = result;
}

void FatFsFake_SetWriteBytesReturned(unsigned int bytes)
{
    writeBytesReturned           = bytes;
    writeBytesReturnedOverridden = true;
}

int FatFsFake_WriteCallCount(void)
{
    return writeCallCount;
}

const void* FatFsFake_LastWriteBuf(void)
{
    return lastWriteBuf;
}

unsigned int FatFsFake_LastWriteCount(void)
{
    return lastWriteCount;
}

FRESULT f_write(FIL* fp, const void* buff, UINT btw, UINT* bw)
{
    (void) fp;
    writeCallCount++;
    lastWriteBuf   = buff;
    lastWriteCount = btw;
    *bw            = writeBytesReturnedOverridden ? writeBytesReturned : btw;
    return writeResult;
}

FRESULT f_sync(FIL* fp)
{
    (void) fp;
    return FR_OK;
}

void FatFsFake_SetStatResult(FRESULT result)
{
    statResult = result;
}

int FatFsFake_StatCallCount(void)
{
    return statCallCount;
}

const char* FatFsFake_LastStatPath(void)
{
    return lastStatPath;
}

FRESULT f_stat(const TCHAR* path, FILINFO* fno)
{
    (void) fno;
    statCallCount++;
    lastStatPath = path;
    return statResult;
}

void FatFsFake_SetUnlinkResult(FRESULT result)
{
    unlinkResult = result;
}

int FatFsFake_UnlinkCallCount(void)
{
    return unlinkCallCount;
}

const char* FatFsFake_LastUnlinkPath(void)
{
    return lastUnlinkPath;
}

FRESULT f_unlink(const TCHAR* path)
{
    unlinkCallCount++;
    lastUnlinkPath = path;
    return unlinkResult;
}
