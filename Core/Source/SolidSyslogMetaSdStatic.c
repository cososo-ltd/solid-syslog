#include "SolidSyslogMetaSd.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogMetaSdErrors.h"
#include "SolidSyslogMetaSdPrivate.h"
#include "SolidSyslogNullSd.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogStructuredData;

static bool MetaSd_IsValidConfig(const struct SolidSyslogMetaSdConfig* config);
static inline size_t MetaSd_IndexFromHandle(const struct SolidSyslogStructuredData* base);
static inline void MetaSd_CleanupAtIndex(size_t index, void* context);

static bool MetaSd_InUse[SOLIDSYSLOG_META_SD_POOL_SIZE];
static struct SolidSyslogMetaSd MetaSd_Pool[SOLIDSYSLOG_META_SD_POOL_SIZE];
static struct SolidSyslogPoolAllocator MetaSd_Allocator = {MetaSd_InUse, SOLIDSYSLOG_META_SD_POOL_SIZE};

struct SolidSyslogStructuredData* SolidSyslogMetaSd_Create(const struct SolidSyslogMetaSdConfig* config)
{
    struct SolidSyslogStructuredData* result = SolidSyslogNullSd_Get();
    if (MetaSd_IsValidConfig(config))
    {
        size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&MetaSd_Allocator);
        if (SolidSyslogPoolAllocator_IndexIsValid(&MetaSd_Allocator, index))
        {
            MetaSd_Initialise(&MetaSd_Pool[index].Base, config);
            result = &MetaSd_Pool[index].Base;
        }
        else
        {
            MetaSd_Report(
                SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
                SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
                METASD_ERROR_POOL_EXHAUSTED
            );
        }
    }
    return result;
}

static bool MetaSd_IsValidConfig(const struct SolidSyslogMetaSdConfig* config)
{
    bool valid = false;
    if (config == NULL)
    {
        MetaSd_Report(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_CAT_BAD_CONFIG, METASD_ERROR_NULL_CONFIG);
    }
    else if (config->Counter == NULL)
    {
        MetaSd_Report(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_CAT_BAD_CONFIG, METASD_ERROR_NULL_COUNTER);
    }
    else
    {
        valid = true;
    }
    return valid;
}

void SolidSyslogMetaSd_Destroy(struct SolidSyslogStructuredData* base)
{
    size_t index = MetaSd_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&MetaSd_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(&MetaSd_Allocator, index, MetaSd_CleanupAtIndex, NULL);
    if (!released)
    {
        MetaSd_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            METASD_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t MetaSd_IndexFromHandle(const struct SolidSyslogStructuredData* base)
{
    size_t result = SOLIDSYSLOG_META_SD_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_META_SD_POOL_SIZE; poolIndex++)
    {
        if (base == &MetaSd_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void MetaSd_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    MetaSd_Cleanup(&MetaSd_Pool[index].Base);
}
