#ifndef SOLIDSYSLOGUDPSENDERPRIVATE_H
#define SOLIDSYSLOGUDPSENDERPRIVATE_H

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogUdpSender.h"

enum SolidSyslogUdpSenderErrors
{
    UDPSENDER_ERROR_NULL_CONFIG,
    UDPSENDER_ERROR_NULL_RESOLVER,
    UDPSENDER_ERROR_NULL_DATAGRAM,
    UDPSENDER_ERROR_NULL_ADDRESS,
    UDPSENDER_ERROR_NULL_ENDPOINT,
    UDPSENDER_ERROR_SEND_NULL_BUFFER,
    UDPSENDER_ERROR_POOL_EXHAUSTED,
    UDPSENDER_ERROR_UNKNOWN_DESTROY,
    UDPSENDER_ERROR_MAX
};

extern const struct SolidSyslogErrorSource UdpSenderErrorSource;

struct SolidSyslogUdpSender
{
    struct SolidSyslogSender Base;
    struct SolidSyslogUdpSenderConfig Config;
    bool Connected;
    uint32_t LastEndpointVersion;
};

void UdpSender_Initialise(struct SolidSyslogSender* base, const struct SolidSyslogUdpSenderConfig* config);
void UdpSender_Cleanup(struct SolidSyslogSender* base);

#endif /* SOLIDSYSLOGUDPSENDERPRIVATE_H */
