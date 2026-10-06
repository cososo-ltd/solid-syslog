#include "vxWorks.h"

#include <sys/stat.h>

#include "VxWorks64FsFake.h"

#include <stdbool.h>
#include <stddef.h>

#include "dosFsLib.h"
#include "hrfsLib.h"

/* A regular file, as stat would report one. */
enum
{
    VXWORKS64FSFAKE_REGULAR_FILE = 0x8000
};

/* What the file system monitor has put on the volume. */
enum VxWorks64FsFake_Volume
{
    VXWORKS64FSFAKE_VOLUME_RAWFS,
    VXWORKS64FSFAKE_VOLUME_ROOT_IS_A_FILE,
    VXWORKS64FSFAKE_VOLUME_DOSFS,
    VXWORKS64FSFAKE_VOLUME_HRFS
};

static enum VxWorks64FsFake_Volume VxWorks64FsFake_TheVolume = VXWORKS64FSFAKE_VOLUME_RAWFS;
static bool VxWorks64FsFake_FormatsFail = false;
static const char* VxWorks64FsFake_FormatPath = NULL;
static int VxWorks64FsFake_FormatOptions = -1;
static bool VxWorks64FsFake_FormatHadNoPrompt = false;
static unsigned VxWorks64FsFake_Stats = 0U;
static const char* VxWorks64FsFake_StatName = NULL;
static unsigned VxWorks64FsFake_Formats = 0U;
static unsigned VxWorks64FsFake_HrfsFormats = 0U;
static unsigned VxWorks64FsFake_VolDescGets = 0U;
static const char* VxWorks64FsFake_VolDescGetName = NULL;
static bool VxWorks64FsFake_VolDescGetHadNoTail = false;
static bool VxWorks64FsFake_HrfsFormatTookTheDefaults = false;

void VxWorks64FsFake_Reset(void)
{
    VxWorks64FsFake_TheVolume = VXWORKS64FSFAKE_VOLUME_RAWFS;
    VxWorks64FsFake_FormatsFail = false;
    VxWorks64FsFake_FormatPath = NULL;
    VxWorks64FsFake_FormatOptions = -1;
    VxWorks64FsFake_FormatHadNoPrompt = false;
    VxWorks64FsFake_Stats = 0U;
    VxWorks64FsFake_StatName = NULL;
    VxWorks64FsFake_Formats = 0U;
    VxWorks64FsFake_HrfsFormats = 0U;
    VxWorks64FsFake_VolDescGets = 0U;
    VxWorks64FsFake_VolDescGetName = NULL;
    VxWorks64FsFake_VolDescGetHadNoTail = false;
    VxWorks64FsFake_HrfsFormatTookTheDefaults = false;
}

void VxWorks64FsFake_FormatTheVolumeAsDosFs(void)
{
    VxWorks64FsFake_TheVolume = VXWORKS64FSFAKE_VOLUME_DOSFS;
}

void VxWorks64FsFake_FormatTheVolumeAsHrfs(void)
{
    VxWorks64FsFake_TheVolume = VXWORKS64FSFAKE_VOLUME_HRFS;
}

void VxWorks64FsFake_MakeTheRootAFile(void)
{
    VxWorks64FsFake_TheVolume = VXWORKS64FSFAKE_VOLUME_ROOT_IS_A_FILE;
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

unsigned VxWorks64FsFake_HrfsFormatCallCount(void)
{
    return VxWorks64FsFake_HrfsFormats;
}

unsigned VxWorks64FsFake_DosFsVolDescGetCallCount(void)
{
    return VxWorks64FsFake_VolDescGets;
}

const char* VxWorks64FsFake_LastVolDescGetName(void)
{
    return VxWorks64FsFake_VolDescGetName;
}

int VxWorks64FsFake_LastVolDescGetHadNoTail(void)
{
    return VxWorks64FsFake_VolDescGetHadNoTail ? 1 : 0;
}

int VxWorks64FsFake_LastHrfsFormatTookTheDefaults(void)
{
    return VxWorks64FsFake_HrfsFormatTookTheDefaults ? 1 : 0;
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

/* rawFs answers no file status; a formatted volume's root is a directory. */
STATUS stat(const char* _name, struct stat* _pStat)
{
    VxWorks64FsFake_Stats++;
    VxWorks64FsFake_StatName = _name;
    bool rootIsAFile = VxWorks64FsFake_TheVolume == VXWORKS64FSFAKE_VOLUME_ROOT_IS_A_FILE;
    bool answers = VxWorks64FsFake_TheVolume != VXWORKS64FSFAKE_VOLUME_RAWFS;
    if (answers)
    {
        _pStat->st_mode = rootIsAFile ? VXWORKS64FSFAKE_REGULAR_FILE : S_IFDIR;
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
        VxWorks64FsFake_TheVolume = VXWORKS64FSFAKE_VOLUME_DOSFS;
    }
    return VxWorks64FsFake_FormatsFail ? ERROR : OK;
}

// NOLINTNEXTLINE(readability-non-const-parameter) -- signature fixed by the VxWorks API
STATUS hrfsFormat(char* path, UINT64 diskSize, UINT32 blkSize, UINT32 numInodes)
{
    VxWorks64FsFake_HrfsFormats++;
    VxWorks64FsFake_HrfsFormatTookTheDefaults = (diskSize == 0U) && (blkSize == 0U) && (numInodes == 0U);
    VxWorks64FsFake_FormatPath = path;
    if (!VxWorks64FsFake_FormatsFail)
    {
        VxWorks64FsFake_TheVolume = VXWORKS64FSFAKE_VOLUME_HRFS;
    }
    return VxWorks64FsFake_FormatsFail ? ERROR : OK;
}

/* Only its address is handed out: a caller never looks inside one. */
struct DOS_VOLUME_DESC
{
    int Unused;
};

static struct DOS_VOLUME_DESC VxWorks64FsFake_DosFsVolume;

/* A descriptor only for a volume dosFs owns; NULL, as for HRFS or rawFs, else. */
DOS_VOLUME_DESC_ID dosFsVolDescGet(void* pDevNameOrPVolDesc, u_char** ppTail)
{
    VxWorks64FsFake_VolDescGets++;
    VxWorks64FsFake_VolDescGetName = (const char*) pDevNameOrPVolDesc;
    VxWorks64FsFake_VolDescGetHadNoTail = ppTail == NULL;
    return (VxWorks64FsFake_TheVolume == VXWORKS64FSFAKE_VOLUME_DOSFS) ? &VxWorks64FsFake_DosFsVolume : NULL;
}
