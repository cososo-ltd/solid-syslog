#include "SolidSyslogPlusFatFile.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullFile.h"
#include "SolidSyslogPlusFatFileErrors.h"
#include "SolidSyslogPlusFatFilePrivate.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogFile;

static inline size_t PlusFatFile_IndexFromHandle(const struct SolidSyslogFile* base);
static inline void PlusFatFile_CleanupAtIndex(size_t index, void* context);

static bool PlusFatFile_InUse[SOLIDSYSLOG_FILE_POOL_SIZE];
static struct SolidSyslogPlusFatFile PlusFatFile_Pool[SOLIDSYSLOG_FILE_POOL_SIZE];
static struct SolidSyslogPoolAllocator PlusFatFile_Allocator = {PlusFatFile_InUse, SOLIDSYSLOG_FILE_POOL_SIZE};

struct SolidSyslogFile* SolidSyslogPlusFatFile_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&PlusFatFile_Allocator);
    struct SolidSyslogFile* handle = SolidSyslogNullFile_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&PlusFatFile_Allocator, index) == true)
    {
        PlusFatFile_Initialise(&PlusFatFile_Pool[index].Base);
        handle = &PlusFatFile_Pool[index].Base;
    }
    else
    {
        PlusFatFile_Report(
            SOLIDSYSLOG_SEVERITY_ERROR,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            PLUSFATFILE_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogPlusFatFile_Destroy(struct SolidSyslogFile* base)
{
    size_t index = PlusFatFile_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&PlusFatFile_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&PlusFatFile_Allocator, index, PlusFatFile_CleanupAtIndex, NULL);
    if (!released)
    {
        PlusFatFile_Report(
            SOLIDSYSLOG_SEVERITY_WARNING,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            PLUSFATFILE_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t PlusFatFile_IndexFromHandle(const struct SolidSyslogFile* base)
{
    size_t result = SOLIDSYSLOG_FILE_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_FILE_POOL_SIZE; poolIndex++)
    {
        if (base == &PlusFatFile_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void PlusFatFile_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    PlusFatFile_Cleanup(&PlusFatFile_Pool[index].Base);
}
