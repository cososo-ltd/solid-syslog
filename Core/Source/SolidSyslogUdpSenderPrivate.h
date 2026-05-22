#ifndef SOLIDSYSLOGUDPSENDERPRIVATE_H
#define SOLIDSYSLOGUDPSENDERPRIVATE_H

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogUdpSender.h"

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
