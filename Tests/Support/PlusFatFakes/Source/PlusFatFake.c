#include "PlusFatFake.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "ff_stdio.h"

enum
{
    OPEN_MODE_CAPACITY = 8
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

void PlusFatFake_Reset(void)
{
    openCallCount = 0;
    lastOpenPath = NULL;
    openFailMode = NULL;
    openAlwaysFails = false;
    memset(openModes, 0, sizeof(openModes));
    closeCallCount = 0;
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
