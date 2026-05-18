#include "SolidSyslogOriginSd.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogNullSd.h"
#include "SolidSyslogOriginSdPrivate.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogStructuredData;

static size_t OriginSd_IndexFromHandle(const struct SolidSyslogStructuredData* base);
static void OriginSd_CleanupAtIndex(size_t index, void* context);

static bool OriginSd_InUse[SOLIDSYSLOG_ORIGIN_SD_POOL_SIZE];
static struct SolidSyslogOriginSd OriginSd_Pool[SOLIDSYSLOG_ORIGIN_SD_POOL_SIZE];
static struct SolidSyslogPoolAllocator OriginSd_Allocator = {OriginSd_InUse, SOLIDSYSLOG_ORIGIN_SD_POOL_SIZE};

struct SolidSyslogStructuredData* SolidSyslogOriginSd_Create(const struct SolidSyslogOriginSdConfig* config)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&OriginSd_Allocator);
    struct SolidSyslogStructuredData* handle = SolidSyslogNullSd_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&OriginSd_Allocator, index))
    {
        OriginSd_Initialise(&OriginSd_Pool[index].Base, config);
        handle = &OriginSd_Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_ORIGINSD_POOL_EXHAUSTED);
    }
    return handle;
}

void SolidSyslogOriginSd_Destroy(struct SolidSyslogStructuredData* base)
{
    size_t index = OriginSd_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&OriginSd_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(&OriginSd_Allocator, index, OriginSd_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_ORIGINSD_UNKNOWN_DESTROY);
    }
}

static size_t OriginSd_IndexFromHandle(const struct SolidSyslogStructuredData* base)
{
    size_t result = SOLIDSYSLOG_ORIGIN_SD_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_ORIGIN_SD_POOL_SIZE; poolIndex++)
    {
        if (base == &OriginSd_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static void OriginSd_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    OriginSd_Cleanup(&OriginSd_Pool[index].Base);
}
