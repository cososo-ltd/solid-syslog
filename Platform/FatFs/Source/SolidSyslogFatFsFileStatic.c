#include "SolidSyslogFatFsFile.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFatFsFileErrors.h"
#include "SolidSyslogFatFsFilePrivate.h"
#include "SolidSyslogNullFile.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogFile;

static inline size_t FatFsFile_IndexFromHandle(const struct SolidSyslogFile* base);
static inline void FatFsFile_CleanupAtIndex(size_t index, void* context);

static bool FatFsFile_InUse[SOLIDSYSLOG_FILE_POOL_SIZE];
static struct SolidSyslogFatFsFile FatFsFile_Pool[SOLIDSYSLOG_FILE_POOL_SIZE];
static struct SolidSyslogPoolAllocator FatFsFile_Allocator = {FatFsFile_InUse, SOLIDSYSLOG_FILE_POOL_SIZE};

struct SolidSyslogFile* SolidSyslogFatFsFile_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&FatFsFile_Allocator);
    struct SolidSyslogFile* handle = SolidSyslogNullFile_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&FatFsFile_Allocator, index) == true)
    {
        FatFsFile_Initialise(&FatFsFile_Pool[index].Base);
        handle = &FatFsFile_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &FatFsFileErrorSource,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            (int32_t) FATFSFILE_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogFatFsFile_Destroy(struct SolidSyslogFile* base)
{
    size_t index = FatFsFile_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&FatFsFile_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(&FatFsFile_Allocator, index, FatFsFile_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &FatFsFileErrorSource,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            (int32_t) FATFSFILE_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t FatFsFile_IndexFromHandle(const struct SolidSyslogFile* base)
{
    size_t result = SOLIDSYSLOG_FILE_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_FILE_POOL_SIZE; poolIndex++)
    {
        if (base == &FatFsFile_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void FatFsFile_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    FatFsFile_Cleanup(&FatFsFile_Pool[index].Base);
}
