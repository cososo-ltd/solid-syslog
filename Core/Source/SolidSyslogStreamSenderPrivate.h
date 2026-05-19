#ifndef SOLIDSYSLOGSTREAMSENDERPRIVATE_H
#define SOLIDSYSLOGSTREAMSENDERPRIVATE_H

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogStreamSender.h"

struct SolidSyslogStreamSender
{
    struct SolidSyslogSender Base;
    struct SolidSyslogStreamSenderConfig Config;
    bool Connected;
    uint32_t LastEndpointVersion;
};

void StreamSender_Initialise(struct SolidSyslogSender* base, const struct SolidSyslogStreamSenderConfig* config);
void StreamSender_Cleanup(struct SolidSyslogSender* base);

#endif /* SOLIDSYSLOGSTREAMSENDERPRIVATE_H */
