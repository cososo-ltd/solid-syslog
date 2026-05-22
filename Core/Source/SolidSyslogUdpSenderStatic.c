#include "SolidSyslogUdpSender.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogNullSender.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogUdpSenderPrivate.h"

struct SolidSyslogSender;

static bool UdpSender_IsValidConfig(const struct SolidSyslogUdpSenderConfig* config);
static inline size_t UdpSender_IndexFromHandle(const struct SolidSyslogSender* base);
static inline void UdpSender_CleanupAtIndex(size_t index, void* context);

static bool UdpSender_InUse[SOLIDSYSLOG_UDP_SENDER_POOL_SIZE];
static struct SolidSyslogUdpSender UdpSender_Pool[SOLIDSYSLOG_UDP_SENDER_POOL_SIZE];
static struct SolidSyslogPoolAllocator UdpSender_Allocator = {UdpSender_InUse, SOLIDSYSLOG_UDP_SENDER_POOL_SIZE};

struct SolidSyslogSender* SolidSyslogUdpSender_Create(const struct SolidSyslogUdpSenderConfig* config)
{
    struct SolidSyslogSender* result = SolidSyslogNullSender_Get();
    if (UdpSender_IsValidConfig(config))
    {
        size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&UdpSender_Allocator);
        if (SolidSyslogPoolAllocator_IndexIsValid(&UdpSender_Allocator, index))
        {
            UdpSender_Initialise(&UdpSender_Pool[index].Base, config);
            result = &UdpSender_Pool[index].Base;
        }
        else
        {
            SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_UDPSENDER_POOL_EXHAUSTED);
        }
    }
    return result;
}

static bool UdpSender_IsValidConfig(const struct SolidSyslogUdpSenderConfig* config)
{
    bool valid = false;
    if (config == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_CONFIG);
    }
    else if (config->Resolver == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_RESOLVER);
    }
    else if (config->Datagram == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_DATAGRAM);
    }
    else if (config->Address == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_ADDRESS);
    }
    else if (config->Endpoint == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_ERROR_MSG_UDPSENDER_CREATE_NULL_ENDPOINT);
    }
    else
    {
        valid = true;
    }
    return valid;
}

void SolidSyslogUdpSender_Destroy(struct SolidSyslogSender* base)
{
    size_t index = UdpSender_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&UdpSender_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(&UdpSender_Allocator, index, UdpSender_CleanupAtIndex, NULL);
    if (!released)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_UDPSENDER_UNKNOWN_DESTROY);
    }
}

static inline size_t UdpSender_IndexFromHandle(const struct SolidSyslogSender* base)
{
    size_t result = SOLIDSYSLOG_UDP_SENDER_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_UDP_SENDER_POOL_SIZE; poolIndex++)
    {
        if (base == &UdpSender_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void UdpSender_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    UdpSender_Cleanup(&UdpSender_Pool[index].Base);
}
