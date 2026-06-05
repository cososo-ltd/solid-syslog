#ifndef SOLIDSYSLOGSTREAMSENDERPRIVATE_H
#define SOLIDSYSLOGSTREAMSENDERPRIVATE_H

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogStreamSenderErrors.h"

struct SolidSyslogStreamSender
{
    struct SolidSyslogSender Base;
    struct SolidSyslogStreamSenderConfig Config;
    bool Connected;
    bool DeliveryHealthy;
    uint32_t LastEndpointVersion;
};

void StreamSender_Initialise(struct SolidSyslogSender* base, const struct SolidSyslogStreamSenderConfig* config);
void StreamSender_Cleanup(struct SolidSyslogSender* base);

static inline void StreamSender_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogStreamSenderErrors code
)
{
    SolidSyslog_Error(severity, &StreamSenderErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGSTREAMSENDERPRIVATE_H */
