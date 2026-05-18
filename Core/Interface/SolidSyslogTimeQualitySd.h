#ifndef SOLIDSYSLOGTIMEQUALITYSD_H
#define SOLIDSYSLOGTIMEQUALITYSD_H

#include "ExternC.h"
#include "SolidSyslogTimeQuality.h"

EXTERN_C_BEGIN

    struct SolidSyslogStructuredData;

    struct SolidSyslogStructuredData* SolidSyslogTimeQualitySd_Create(SolidSyslogTimeQualityFunction getTimeQuality);
    void SolidSyslogTimeQualitySd_Destroy(struct SolidSyslogStructuredData * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGTIMEQUALITYSD_H */
