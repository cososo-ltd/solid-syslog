#include "PlusFatFake.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "ff_stdio.h"

enum
{
    OPEN_MODE_CAPACITY = 8,
    READ_SOURCE_CAPACITY = 256
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

void PlusFatFake_Reset(void)
{
    openCallCount = 0;
    lastOpenPath = NULL;
    openFailMode = NULL;
    openAlwaysFails = false;
    memset(openModes, 0, sizeof(openModes));
    closeCallCount = 0;
    memset(readSource, 0, sizeof(readSource));
    readSourceCount = 0;
    readCallCount = 0;
    lastReadSize = 0;
    lastReadItems = 0;
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
