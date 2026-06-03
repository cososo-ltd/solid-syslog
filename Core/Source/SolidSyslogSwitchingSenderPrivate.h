#ifndef SOLIDSYSLOGSWITCHINGSENDERPRIVATE_H
#define SOLIDSYSLOGSWITCHINGSENDERPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogSwitchingSender.h"
#include "SolidSyslogSwitchingSenderErrors.h"

struct SolidSyslogSwitchingSender
{
    struct SolidSyslogSender Base;
    struct SolidSyslogSwitchingSenderConfig Config;
    struct SolidSyslogSender* CurrentSender;
};

void SwitchingSender_Initialise(struct SolidSyslogSender* base, const struct SolidSyslogSwitchingSenderConfig* config);
void SwitchingSender_Cleanup(struct SolidSyslogSender* base);

static inline void SwitchingSender_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogSwitchingSenderErrors code
)
{
    SolidSyslog_Error(severity, &SwitchingSenderErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGSWITCHINGSENDERPRIVATE_H */
