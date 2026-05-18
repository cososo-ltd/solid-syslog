#ifndef SOLIDSYSLOGTIMEQUALITYSDPRIVATE_H
#define SOLIDSYSLOGTIMEQUALITYSDPRIVATE_H

#include "SolidSyslogStructuredDataDefinition.h"
#include "SolidSyslogTimeQuality.h"

struct SolidSyslogTimeQualitySd
{
    struct SolidSyslogStructuredData Base;
    SolidSyslogTimeQualityFunction GetTimeQuality;
};

void TimeQualitySd_Initialise(struct SolidSyslogStructuredData* base, SolidSyslogTimeQualityFunction getTimeQuality);
void TimeQualitySd_Cleanup(struct SolidSyslogStructuredData* base);

#endif /* SOLIDSYSLOGTIMEQUALITYSDPRIVATE_H */
