#ifndef SOLIDSYSLOGFREERTOSRESOLVERPRIVATE_H
#define SOLIDSYSLOGFREERTOSRESOLVERPRIVATE_H

#include "SolidSyslogResolverDefinition.h"

struct SolidSyslogFreeRtosResolver
{
    struct SolidSyslogResolver Base;
};

void FreeRtosResolver_Initialise(struct SolidSyslogResolver* base);
void FreeRtosResolver_Cleanup(struct SolidSyslogResolver* base);

#endif /* SOLIDSYSLOGFREERTOSRESOLVERPRIVATE_H */
