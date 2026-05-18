#ifndef SOLIDSYSLOGMETASDPRIVATE_H
#define SOLIDSYSLOGMETASDPRIVATE_H

#include "SolidSyslogMetaSd.h"
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

#endif /* SOLIDSYSLOGMETASDPRIVATE_H */
