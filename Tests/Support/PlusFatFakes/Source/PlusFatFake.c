#include "PlusFatFake.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "ff_stdio.h"

enum
{
    OPEN_MODE_CAPACITY = 8,
    READ_SOURCE_CAPACITY = 256,
    WRITE_CAPTURE_CAPACITY = 256
};

static FF_FILE fakeFile;

/* ff_fopen state */
static int openCallCount;
static const char* openModes[OPEN_MODE_CAPACITY];
static const char* lastOpenPath;
static const char* openFailMode;
static bool openAlwaysFails;

/* ff_fclose state */
static int closeCallCount;

/* ff_fread state */
static unsigned char readSource[READ_SOURCE_CAPACITY];
static size_t readSourceCount;
static int readCallCount;
static size_t lastReadSize;
static size_t lastReadItems;

/* ff_fwrite state */
static int writeCallCount;
static unsigned char lastWriteBytes[WRITE_CAPTURE_CAPACITY];
static size_t lastWriteItems;
static bool writeIncomplete;

/* FF_FlushCache state */
static int flushCacheCallCount;
static int flushCacheResult;

/* ff_fseek state */
static int seekCallCount;
static long lastSeekOffset;
static int lastSeekWhence;

/* ff_filelength state */
static size_t fileLength;

/* ff_seteof state */
static int seteofCallCount;

/* ff_stat state */
static int statCallCount;
static const char* lastStatPath;
static int statResult;

/* ff_remove state */
static int removeCallCount;
static const char* lastRemovePath;
static int removeResult;

void PlusFatFake_Reset(void)
{
    openCallCount = 0;
    lastOpenPath = NULL;
    openFailMode = NULL;
    openAlwaysFails = false;
    for (size_t modeIndex = 0; modeIndex < OPEN_MODE_CAPACITY; modeIndex++)
    {
        openModes[modeIndex] = NULL;
    }
    closeCallCount = 0;
    memset(readSource, 0, sizeof(readSource));
    readSourceCount = 0;
    readCallCount = 0;
    lastReadSize = 0;
    lastReadItems = 0;
    writeCallCount = 0;
    memset(lastWriteBytes, 0, sizeof(lastWriteBytes));
    lastWriteItems = 0;
    writeIncomplete = false;
    flushCacheCallCount = 0;
    flushCacheResult = 0;
    seekCallCount = 0;
    lastSeekOffset = 0;
    lastSeekWhence = 0;
    fileLength = 0;
    seteofCallCount = 0;
    statCallCount = 0;
    lastStatPath = NULL;
    statResult = 0;
    removeCallCount = 0;
    lastRemovePath = NULL;
    removeResult = 0;
}

void PlusFatFake_SetOpenFailsForMode(const char* mode)
{
    openFailMode = mode;
}

void PlusFatFake_SetOpenAlwaysFails(void)
{
    openAlwaysFails = true;
}

int PlusFatFake_OpenCallCount(void)
{
    return openCallCount;
}

const char* PlusFatFake_OpenModeAt(int index)
{
    return openModes[index];
}

const char* PlusFatFake_LastOpenPath(void)
{
    return lastOpenPath;
}

FF_FILE* ff_fopen(const char* pcFile, const char* pcMode)
{
    if (openCallCount < OPEN_MODE_CAPACITY)
    {
        openModes[openCallCount] = pcMode;
    }
    lastOpenPath = pcFile;
    openCallCount++;

    FF_FILE* result = &fakeFile;
    if (openAlwaysFails || ((openFailMode != NULL) && (strcmp(pcMode, openFailMode) == 0)))
    {
        result = NULL;
    }
    return result;
}

int PlusFatFake_CloseCallCount(void)
{
    return closeCallCount;
}

int ff_fclose(FF_FILE* pxStream)
{
    (void) pxStream;
    closeCallCount++;
    return 0;
}

void PlusFatFake_SetReadSource(const void* bytes, unsigned long count)
{
    size_t copyCount = (count <= sizeof(readSource)) ? (size_t) count : sizeof(readSource);
    memcpy(readSource, bytes, copyCount);
    readSourceCount = copyCount;
}

/* The adapter always calls ff_fread with xSize == 1, so item count == byte
 * count. Copies from the programmed source and returns the number of items
 * (bytes) delivered, capped at the programmed source length. */
int PlusFatFake_ReadCallCount(void)
{
    return readCallCount;
}

unsigned long PlusFatFake_LastReadSize(void)
{
    return (unsigned long) lastReadSize;
}

unsigned long PlusFatFake_LastReadItems(void)
{
    return (unsigned long) lastReadItems;
}

size_t ff_fread(void* pvBuffer, size_t xSize, size_t xItems, FF_FILE* pxStream)
{
    (void) pxStream;
    readCallCount++;
    lastReadSize = xSize;
    lastReadItems = xItems;
    size_t itemsDelivered = (xItems <= readSourceCount) ? xItems : readSourceCount;
    memcpy(pvBuffer, readSource, itemsDelivered);
    return itemsDelivered;
}

void PlusFatFake_SetWriteIncomplete(void)
{
    writeIncomplete = true;
}

int PlusFatFake_WriteCallCount(void)
{
    return writeCallCount;
}

const void* PlusFatFake_LastWriteBytes(void)
{
    return lastWriteBytes;
}

unsigned long PlusFatFake_LastWriteItems(void)
{
    return (unsigned long) lastWriteItems;
}

size_t ff_fwrite(const void* pvBuffer, size_t xSize, size_t xItems, FF_FILE* pxStream)
{
    (void) xSize;
    (void) pxStream;
    writeCallCount++;
    size_t copyCount = (xItems <= sizeof(lastWriteBytes)) ? xItems : sizeof(lastWriteBytes);
    memcpy(lastWriteBytes, pvBuffer, copyCount);
    lastWriteItems = xItems;
    size_t itemsWritten = xItems;
    if (writeIncomplete && (xItems > 0))
    {
        itemsWritten = xItems - 1;
    }
    return itemsWritten;
}

void PlusFatFake_SetFlushCacheFails(void)
{
    /* Any error code with the FF_ERRFLAG bit set is non-FF_ERR_NONE — the
     * adapter treats it as a flush failure. */
    flushCacheResult = (int) (FF_ERR_IOMAN_DRIVER_FATAL_ERROR | FF_ERRFLAG);
}

int PlusFatFake_FlushCacheCallCount(void)
{
    return flushCacheCallCount;
}

FF_Error_t FF_FlushCache(FF_IOManager_t* pxIOManager)
{
    (void) pxIOManager;
    flushCacheCallCount++;
    return flushCacheResult;
}

int PlusFatFake_SeekCallCount(void)
{
    return seekCallCount;
}

long PlusFatFake_LastSeekOffset(void)
{
    return lastSeekOffset;
}

int PlusFatFake_LastSeekWhence(void)
{
    return lastSeekWhence;
}

int ff_fseek(FF_FILE* pxStream, long lOffset, int iWhence)
{
    (void) pxStream;
    seekCallCount++;
    lastSeekOffset = lOffset;
    lastSeekWhence = iWhence;
    return 0;
}

void PlusFatFake_SetFileLength(unsigned long length)
{
    fileLength = (size_t) length;
}

size_t ff_filelength(FF_FILE* pxFile)
{
    (void) pxFile;
    return fileLength;
}

int PlusFatFake_SeteofCallCount(void)
{
    return seteofCallCount;
}

int ff_seteof(FF_FILE* pxStream)
{
    (void) pxStream;
    seteofCallCount++;
    return 0;
}

void PlusFatFake_SetStatFails(void)
{
    statResult = -1;
}

int PlusFatFake_StatCallCount(void)
{
    return statCallCount;
}

const char* PlusFatFake_LastStatPath(void)
{
    return lastStatPath;
}

int ff_stat(const char* pcFileName, FF_Stat_t* pxStatBuffer)
{
    (void) pxStatBuffer;
    statCallCount++;
    lastStatPath = pcFileName;
    return statResult;
}

void PlusFatFake_SetRemoveFails(void)
{
    removeResult = -1;
}

int PlusFatFake_RemoveCallCount(void)
{
    return removeCallCount;
}

const char* PlusFatFake_LastRemovePath(void)
{
    return lastRemovePath;
}

int ff_remove(const char* pcPath)
{
    removeCallCount++;
    lastRemovePath = pcPath;
    return removeResult;
}
