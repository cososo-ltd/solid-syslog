#ifndef SOLIDSYSLOGSWITCHINGSENDERPRIVATE_H
#define SOLIDSYSLOGSWITCHINGSENDERPRIVATE_H

#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogSwitchingSender.h"

struct SolidSyslogSwitchingSender
{
    struct SolidSyslogSender Base;
    struct SolidSyslogSwitchingSenderConfig Config;
    struct SolidSyslogSender* CurrentSender;
};

void SwitchingSender_Initialise(struct SolidSyslogSender* base, const struct SolidSyslogSwitchingSenderConfig* config);
void SwitchingSender_Cleanup(struct SolidSyslogSender* base);

#endif /* SOLIDSYSLOGSWITCHINGSENDERPRIVATE_H */
