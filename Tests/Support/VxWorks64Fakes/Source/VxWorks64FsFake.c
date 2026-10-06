#include "vxWorks.h"

#include <sys/stat.h>

#include "VxWorks64FsFake.h"

#include <stdbool.h>
#include <stddef.h>

#include "dosFsLib.h"

/* A regular file, as stat would report one. */
enum
{
    VXWORKS64FSFAKE_REGULAR_FILE = 0x8000
};

static bool VxWorks64FsFake_IsDosFs = false;
static bool VxWorks64FsFake_RootIsAFile = false;
static bool VxWorks64FsFake_FormatsFail = false;
static const char* VxWorks64FsFake_FormatPath = NULL;
static int VxWorks64FsFake_FormatOptions = -1;
static bool VxWorks64FsFake_FormatHadNoPrompt = false;
static unsigned VxWorks64FsFake_Stats = 0U;
static const char* VxWorks64FsFake_StatName = NULL;
static unsigned VxWorks64FsFake_Formats = 0U;

void VxWorks64FsFake_Reset(void)
{
    VxWorks64FsFake_IsDosFs = false;
    VxWorks64FsFake_RootIsAFile = false;
    VxWorks64FsFake_FormatsFail = false;
    VxWorks64FsFake_FormatPath = NULL;
    VxWorks64FsFake_FormatOptions = -1;
    VxWorks64FsFake_FormatHadNoPrompt = false;
    VxWorks64FsFake_Stats = 0U;
    VxWorks64FsFake_StatName = NULL;
    VxWorks64FsFake_Formats = 0U;
}

void VxWorks64FsFake_FormatTheVolume(void)
{
    VxWorks64FsFake_IsDosFs = true;
}

void VxWorks64FsFake_MakeTheRootAFile(void)
{
    VxWorks64FsFake_RootIsAFile = true;
}

void VxWorks64FsFake_FailFormats(void)
{
    VxWorks64FsFake_FormatsFail = true;
}

unsigned VxWorks64FsFake_StatCallCount(void)
{
    return VxWorks64FsFake_Stats;
}

const char* VxWorks64FsFake_LastStatName(void)
{
    return VxWorks64FsFake_StatName;
}

unsigned VxWorks64FsFake_DosFsVolFormatCallCount(void)
{
    return VxWorks64FsFake_Formats;
}

const char* VxWorks64FsFake_LastFormatPath(void)
{
    return VxWorks64FsFake_FormatPath;
}

int VxWorks64FsFake_LastFormatOptions(void)
{
    return VxWorks64FsFake_FormatOptions;
}

int VxWorks64FsFake_LastFormatHadNoPrompt(void)
{
    return VxWorks64FsFake_FormatHadNoPrompt ? 1 : 0;
}

/* rawFs answers no file status; a dosFs volume's root is a directory. */
STATUS stat(const char* _name, struct stat* _pStat)
{
    VxWorks64FsFake_Stats++;
    VxWorks64FsFake_StatName = _name;
    bool answers = VxWorks64FsFake_IsDosFs || VxWorks64FsFake_RootIsAFile;
    if (answers)
    {
        _pStat->st_mode = VxWorks64FsFake_RootIsAFile ? VXWORKS64FSFAKE_REGULAR_FILE : S_IFDIR;
    }
    return answers ? OK : ERROR;
}

// NOLINTNEXTLINE(readability-non-const-parameter) -- signature fixed by the VxWorks API
STATUS dosFsVolFormat(char* path, int opt, FUNCPTR pPromptFunc)
{
    VxWorks64FsFake_Formats++;
    VxWorks64FsFake_FormatPath = path;
    VxWorks64FsFake_FormatOptions = opt;
    VxWorks64FsFake_FormatHadNoPrompt = pPromptFunc == NULL;
    if (!VxWorks64FsFake_FormatsFail)
    {
        VxWorks64FsFake_IsDosFs = true;
        VxWorks64FsFake_RootIsAFile = false;
    }
    return VxWorks64FsFake_FormatsFail ? ERROR : OK;
}
