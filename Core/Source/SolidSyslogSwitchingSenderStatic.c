#include "SolidSyslogSwitchingSender.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogNullSender.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSwitchingSenderPrivate.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogSender;

static bool SwitchingSender_IsValidConfig(const struct SolidSyslogSwitchingSenderConfig* config);
static size_t SwitchingSender_IndexFromHandle(const struct SolidSyslogSender* base);
static void SwitchingSender_CleanupAtIndex(size_t index, void* context);

static bool SwitchingSender_InUse[SOLIDSYSLOG_SWITCHING_SENDER_POOL_SIZE];
static struct SolidSyslogSwitchingSender SwitchingSender_Pool[SOLIDSYSLOG_SWITCHING_SENDER_POOL_SIZE];
static struct SolidSyslogPoolAllocator SwitchingSender_Allocator = {
    SwitchingSender_InUse,
    SOLIDSYSLOG_SWITCHING_SENDER_POOL_SIZE
};

struct SolidSyslogSender* SolidSyslogSwitchingSender_Create(const struct SolidSyslogSwitchingSenderConfig* config)
{
    struct SolidSyslogSender* handle = SolidSyslogNullSender_Get();
    if (SwitchingSender_IsValidConfig(config))
    {
        size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&SwitchingSender_Allocator);
        if (SolidSyslogPoolAllocator_IndexIsValid(&SwitchingSender_Allocator, index))
        {
            SwitchingSender_Initialise(&SwitchingSender_Pool[index].Base, config);
            handle = &SwitchingSender_Pool[index].Base;
        }
        else
        {
            SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_SWITCHINGSENDER_POOL_EXHAUSTED);
        }
    }
    return handle;
}

static bool SwitchingSender_IsValidConfig(const struct SolidSyslogSwitchingSenderConfig* config)
{
    bool valid = false;
    if (config == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_SWITCHINGSENDER_CREATE_NULL_CONFIG);
    }
    else if (config->Senders == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_SWITCHINGSENDER_CREATE_NULL_SENDERS);
    }
    else if (config->Selector == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_SWITCHINGSENDER_CREATE_NULL_SELECTOR);
    }
    else
    {
        valid = true;
    }
    return valid;
}

void SolidSyslogSwitchingSender_Destroy(struct SolidSyslogSender* base)
{
    size_t index = SwitchingSender_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&SwitchingSender_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&SwitchingSender_Allocator, index, SwitchingSender_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_SWITCHINGSENDER_UNKNOWN_DESTROY);
    }
}

static size_t SwitchingSender_IndexFromHandle(const struct SolidSyslogSender* base)
{
    size_t result = SOLIDSYSLOG_SWITCHING_SENDER_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_SWITCHING_SENDER_POOL_SIZE; poolIndex++)
    {
        if (base == &SwitchingSender_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static void SwitchingSender_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SwitchingSender_Cleanup(&SwitchingSender_Pool[index].Base);
}
