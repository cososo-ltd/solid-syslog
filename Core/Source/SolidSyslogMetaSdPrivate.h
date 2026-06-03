#ifndef SOLIDSYSLOGMETASDPRIVATE_H
#define SOLIDSYSLOGMETASDPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogMetaSdErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStringFunction.h"
#include "SolidSyslogStructuredDataDefinition.h"

struct SolidSyslogAtomicCounter;

struct SolidSyslogMetaSd
{
    struct SolidSyslogStructuredData Base;
    struct SolidSyslogAtomicCounter* Counter;
    SolidSyslogSysUpTimeFunction GetSysUpTime;
    SolidSyslogStringFunction GetLanguage;
};

void MetaSd_Initialise(struct SolidSyslogStructuredData* base, const struct SolidSyslogMetaSdConfig* config);
void MetaSd_Cleanup(struct SolidSyslogStructuredData* base);

static inline void MetaSd_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogMetaSdErrors code
)
{
    SolidSyslog_Error(severity, &MetaSdErrorSource, category, code);
}

#endif /* SOLIDSYSLOGMETASDPRIVATE_H */
