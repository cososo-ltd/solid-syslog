#include "SolidSyslogPosixFile.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullFile.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPosixFileErrors.h"
#include "SolidSyslogPosixFilePrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogFile;

static inline size_t PosixFile_IndexFromHandle(const struct SolidSyslogFile* base);
static inline void PosixFile_CleanupAtIndex(size_t index, void* context);

static bool PosixFile_InUse[SOLIDSYSLOG_FILE_POOL_SIZE];
static struct SolidSyslogPosixFile PosixFile_Pool[SOLIDSYSLOG_FILE_POOL_SIZE];
static struct SolidSyslogPoolAllocator PosixFile_Allocator = {PosixFile_InUse, SOLIDSYSLOG_FILE_POOL_SIZE};

struct SolidSyslogFile* SolidSyslogPosixFile_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&PosixFile_Allocator);
    struct SolidSyslogFile* handle = SolidSyslogNullFile_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&PosixFile_Allocator, index) == true)
    {
        PosixFile_Initialise(&PosixFile_Pool[index].Base);
        handle = &PosixFile_Pool[index].Base;
    }
    else
    {
        PosixFile_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            POSIXFILE_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogPosixFile_Destroy(struct SolidSyslogFile* base)
{
    size_t index = PosixFile_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&PosixFile_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(&PosixFile_Allocator, index, PosixFile_CleanupAtIndex, NULL);
    if (!released)
    {
        PosixFile_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            POSIXFILE_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t PosixFile_IndexFromHandle(const struct SolidSyslogFile* base)
{
    size_t result = SOLIDSYSLOG_FILE_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_FILE_POOL_SIZE; poolIndex++)
    {
        if (base == &PosixFile_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void PosixFile_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    PosixFile_Cleanup(&PosixFile_Pool[index].Base);
}
