#include "SolidSyslogSwitchingSender.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogNullSender.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogSwitchingSenderPrivate.h"
#include "SolidSyslogTunables.h"

static size_t SwitchingSender_IndexFromHandle(const struct SolidSyslogSender* base);
static void SwitchingSender_CleanupAtIndex(size_t index, void* context);

static bool InUse[SOLIDSYSLOG_SWITCHING_SENDER_POOL_SIZE];
static struct SolidSyslogSwitchingSender Pool[SOLIDSYSLOG_SWITCHING_SENDER_POOL_SIZE];
static struct SolidSyslogPoolAllocator Allocator = {InUse, SOLIDSYSLOG_SWITCHING_SENDER_POOL_SIZE};

struct SolidSyslogSender* SolidSyslogSwitchingSender_Create(const struct SolidSyslogSwitchingSenderConfig* config)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&Allocator);
    struct SolidSyslogSender* handle = SolidSyslogNullSender_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&Allocator, index))
    {
        SwitchingSender_Initialise(&Pool[index].Base, config);
        handle = &Pool[index].Base;
    }
    else
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_SWITCHINGSENDER_POOL_EXHAUSTED);
    }
    return handle;
}

void SolidSyslogSwitchingSender_Destroy(struct SolidSyslogSender* base)
{
    size_t index = SwitchingSender_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(&Allocator, index, SwitchingSender_CleanupAtIndex, NULL);
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
        if (base == &Pool[poolIndex].Base)
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
    SwitchingSender_Cleanup(&Pool[index].Base);
}
