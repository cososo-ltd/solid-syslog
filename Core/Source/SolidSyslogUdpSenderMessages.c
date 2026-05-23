#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogUdpSenderPrivate.h"

static const char* UdpSenderError_AsString(uint8_t code)
{
    static const char* const messages[UDPSENDER_ERROR_MAX] = {
        [UDPSENDER_ERROR_NULL_CONFIG] = "SolidSyslogUdpSender_Create called with NULL config",
        [UDPSENDER_ERROR_NULL_RESOLVER] = "SolidSyslogUdpSender_Create config.Resolver is NULL",
        [UDPSENDER_ERROR_NULL_DATAGRAM] = "SolidSyslogUdpSender_Create config.Datagram is NULL",
        [UDPSENDER_ERROR_NULL_ADDRESS] = "SolidSyslogUdpSender_Create config.Address is NULL",
        [UDPSENDER_ERROR_NULL_ENDPOINT] = "SolidSyslogUdpSender_Create config.Endpoint is NULL",
        [UDPSENDER_ERROR_SEND_NULL_BUFFER] = "SolidSyslogUdpSender_Send called with NULL buffer",
        [UDPSENDER_ERROR_POOL_EXHAUSTED] = "SolidSyslogUdpSender_Create pool exhausted; returning fallback sender",
        [UDPSENDER_ERROR_UNKNOWN_DESTROY] = "SolidSyslogUdpSender_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) UDPSENDER_ERROR_MAX)
    {
        enum SolidSyslogUdpSenderErrors typed = (enum SolidSyslogUdpSenderErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource UdpSenderErrorSource = {"UdpSender", UdpSenderError_AsString};
