#include "SolidSyslogMetaSd.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogMetaSdPrivate.h"
#include "SolidSyslogNullSd.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogStructuredData;

static bool MetaSd_IsValidConfig(const struct SolidSyslogMetaSdConfig* config);
static size_t MetaSd_IndexFromHandle(const struct SolidSyslogStructuredData* base);
static void MetaSd_CleanupAtIndex(size_t index, void* context);

static bool InUse[SOLIDSYSLOG_META_SD_POOL_SIZE];
static struct SolidSyslogMetaSd Pool[SOLIDSYSLOG_META_SD_POOL_SIZE];
static struct SolidSyslogPoolAllocator Allocator = {InUse, SOLIDSYSLOG_META_SD_POOL_SIZE};

struct SolidSyslogStructuredData* SolidSyslogMetaSd_Create(const struct SolidSyslogMetaSdConfig* config)
{
    struct SolidSyslogStructuredData* result = SolidSyslogNullSd_Get();
    if (MetaSd_IsValidConfig(config))
    {
        size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&Allocator);
        if (SolidSyslogPoolAllocator_IndexIsValid(&Allocator, index))
        {
            MetaSd_Initialise(&Pool[index].Base, config);
            result = &Pool[index].Base;
        }
        else
        {
            SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_METASD_POOL_EXHAUSTED);
        }
    }
    return result;
}

void SolidSyslogMetaSd_Destroy(struct SolidSyslogStructuredData* base)
{
    size_t index = MetaSd_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(&Allocator, index, MetaSd_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_METASD_UNKNOWN_DESTROY);
    }
}

static bool MetaSd_IsValidConfig(const struct SolidSyslogMetaSdConfig* config)
{
    bool valid = false;
    if (config == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_METASD_CREATE_NULL_CONFIG);
    }
    else if (config->Counter == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_METASD_CREATE_NULL_COUNTER);
    }
    else
    {
        valid = true;
    }
    return valid;
}

static size_t MetaSd_IndexFromHandle(const struct SolidSyslogStructuredData* base)
{
    size_t result = SOLIDSYSLOG_META_SD_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_META_SD_POOL_SIZE; poolIndex++)
    {
        if (base == &Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static void MetaSd_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    MetaSd_Cleanup(&Pool[index].Base);
}
