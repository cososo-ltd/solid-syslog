#include "SolidSyslogFileBlockDevice.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFileBlockDeviceErrors.h"
#include "SolidSyslogFileBlockDevicePrivate.h"
#include "SolidSyslogNullBlockDevice.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogBlockDevice;
struct SolidSyslogFile;

static inline size_t FileBlockDevice_IndexFromHandle(const struct SolidSyslogBlockDevice* base);
static inline void FileBlockDevice_CleanupAtIndex(size_t index, void* context);

static bool FileBlockDevice_InUse[SOLIDSYSLOG_FILE_BLOCK_DEVICE_POOL_SIZE];
static struct SolidSyslogFileBlockDevice FileBlockDevice_Pool[SOLIDSYSLOG_FILE_BLOCK_DEVICE_POOL_SIZE];
static struct SolidSyslogPoolAllocator FileBlockDevice_Allocator = {
    FileBlockDevice_InUse,
    SOLIDSYSLOG_FILE_BLOCK_DEVICE_POOL_SIZE
};

struct SolidSyslogBlockDevice* SolidSyslogFileBlockDevice_Create(struct SolidSyslogFile* file, const char* pathPrefix)
{
    struct SolidSyslogBlockDevice* result = SolidSyslogNullBlockDevice_Get();
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&FileBlockDevice_Allocator);
    if (SolidSyslogPoolAllocator_IndexIsValid(&FileBlockDevice_Allocator, index))
    {
        FileBlockDevice_Initialise(&FileBlockDevice_Pool[index].Base, file, pathPrefix);
        result = &FileBlockDevice_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &FileBlockDeviceErrorSource,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            (int32_t) FILEBLOCKDEVICE_ERROR_POOL_EXHAUSTED
        );
    }
    return result;
}

void SolidSyslogFileBlockDevice_Destroy(struct SolidSyslogBlockDevice* base)
{
    size_t index = FileBlockDevice_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&FileBlockDevice_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&FileBlockDevice_Allocator, index, FileBlockDevice_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_WARNING,
            &FileBlockDeviceErrorSource,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            (int32_t) FILEBLOCKDEVICE_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t FileBlockDevice_IndexFromHandle(const struct SolidSyslogBlockDevice* base)
{
    size_t result = SOLIDSYSLOG_FILE_BLOCK_DEVICE_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_FILE_BLOCK_DEVICE_POOL_SIZE; poolIndex++)
    {
        if (base == &FileBlockDevice_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void FileBlockDevice_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    FileBlockDevice_Cleanup(&FileBlockDevice_Pool[index].Base);
}
