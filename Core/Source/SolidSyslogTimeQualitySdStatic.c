#include "SolidSyslogTimeQualitySd.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogNullSd.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTimeQuality.h"
#include "SolidSyslogTimeQualitySdPrivate.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogStructuredData;

static size_t TimeQualitySd_IndexFromHandle(const struct SolidSyslogStructuredData* base);
static void TimeQualitySd_CleanupAtIndex(size_t index, void* context);

static bool InUse[SOLIDSYSLOG_TIME_QUALITY_SD_POOL_SIZE];
static struct SolidSyslogTimeQualitySd Pool[SOLIDSYSLOG_TIME_QUALITY_SD_POOL_SIZE];
static struct SolidSyslogPoolAllocator Allocator = {InUse, SOLIDSYSLOG_TIME_QUALITY_SD_POOL_SIZE};

struct SolidSyslogStructuredData* SolidSyslogTimeQualitySd_Create(SolidSyslogTimeQualityFunction getTimeQuality)
{
    struct SolidSyslogStructuredData* handle = SolidSyslogNullSd_Get();
    if (getTimeQuality == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_TIMEQUALITYSD_CREATE_NULL_CALLBACK);
    }
    else
    {
        size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&Allocator);
        if (SolidSyslogPoolAllocator_IndexIsValid(&Allocator, index))
        {
            TimeQualitySd_Initialise(&Pool[index].Base, getTimeQuality);
            handle = &Pool[index].Base;
        }
        else
        {
            SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_TIMEQUALITYSD_POOL_EXHAUSTED);
        }
    }
    return handle;
}

void SolidSyslogTimeQualitySd_Destroy(struct SolidSyslogStructuredData* base)
{
    size_t index = TimeQualitySd_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(&Allocator, index, TimeQualitySd_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_TIMEQUALITYSD_UNKNOWN_DESTROY);
    }
}

static size_t TimeQualitySd_IndexFromHandle(const struct SolidSyslogStructuredData* base)
{
    size_t result = SOLIDSYSLOG_TIME_QUALITY_SD_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_TIME_QUALITY_SD_POOL_SIZE; poolIndex++)
    {
        if (base == &Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static void TimeQualitySd_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    TimeQualitySd_Cleanup(&Pool[index].Base);
}
