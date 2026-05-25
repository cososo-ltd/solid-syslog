#ifndef SOLIDSYSLOGPLUSTCPRESOLVERPRIVATE_H
#define SOLIDSYSLOGPLUSTCPRESOLVERPRIVATE_H

#include "SolidSyslogResolverDefinition.h"

struct SolidSyslogPlusTcpResolver
{
    struct SolidSyslogResolver Base;
};

void PlusTcpResolver_Initialise(struct SolidSyslogResolver* base);
void PlusTcpResolver_Cleanup(struct SolidSyslogResolver* base);

#endif /* SOLIDSYSLOGPLUSTCPRESOLVERPRIVATE_H */
