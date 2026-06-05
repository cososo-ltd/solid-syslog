#include "SolidSyslogTimeQualitySd.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullSd.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTimeQuality.h"
#include "SolidSyslogTimeQualitySdErrors.h"
#include "SolidSyslogTimeQualitySdPrivate.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogStructuredData;

static inline size_t TimeQualitySd_IndexFromHandle(const struct SolidSyslogStructuredData* base);
static inline void TimeQualitySd_CleanupAtIndex(size_t index, void* context);

static bool TimeQualitySd_InUse[SOLIDSYSLOG_TIME_QUALITY_SD_POOL_SIZE];
static struct SolidSyslogTimeQualitySd TimeQualitySd_Pool[SOLIDSYSLOG_TIME_QUALITY_SD_POOL_SIZE];
static struct SolidSyslogPoolAllocator TimeQualitySd_Allocator = {
    TimeQualitySd_InUse,
    SOLIDSYSLOG_TIME_QUALITY_SD_POOL_SIZE
};

struct SolidSyslogStructuredData* SolidSyslogTimeQualitySd_Create(SolidSyslogTimeQualityFunction getTimeQuality)
{
    struct SolidSyslogStructuredData* handle = SolidSyslogNullSd_Get();
    if (getTimeQuality == NULL)
    {
        TimeQualitySd_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            TIMEQUALITYSD_ERROR_NULL_CALLBACK
        );
    }
    else
    {
        size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&TimeQualitySd_Allocator);
        if (SolidSyslogPoolAllocator_IndexIsValid(&TimeQualitySd_Allocator, index))
        {
            TimeQualitySd_Initialise(&TimeQualitySd_Pool[index].Base, getTimeQuality);
            handle = &TimeQualitySd_Pool[index].Base;
        }
        else
        {
            TimeQualitySd_Report(
                SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
                SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
                TIMEQUALITYSD_ERROR_POOL_EXHAUSTED
            );
        }
    }
    return handle;
}

void SolidSyslogTimeQualitySd_Destroy(struct SolidSyslogStructuredData* base)
{
    size_t index = TimeQualitySd_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&TimeQualitySd_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&TimeQualitySd_Allocator, index, TimeQualitySd_CleanupAtIndex, NULL);
    if (!released)
    {
        TimeQualitySd_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            TIMEQUALITYSD_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t TimeQualitySd_IndexFromHandle(const struct SolidSyslogStructuredData* base)
{
    size_t result = SOLIDSYSLOG_TIME_QUALITY_SD_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_TIME_QUALITY_SD_POOL_SIZE; poolIndex++)
    {
        if (base == &TimeQualitySd_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void TimeQualitySd_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    TimeQualitySd_Cleanup(&TimeQualitySd_Pool[index].Base);
}
