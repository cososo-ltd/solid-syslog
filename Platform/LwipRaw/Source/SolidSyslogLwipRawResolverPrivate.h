#ifndef SOLIDSYSLOGLWIPRAWRESOLVERPRIVATE_H
#define SOLIDSYSLOGLWIPRAWRESOLVERPRIVATE_H

#include "SolidSyslogResolverDefinition.h"

struct SolidSyslogLwipRawResolver
{
    struct SolidSyslogResolver Base;
};

void LwipRawResolver_Initialise(struct SolidSyslogResolver* base);
void LwipRawResolver_Cleanup(struct SolidSyslogResolver* base);

#endif /* SOLIDSYSLOGLWIPRAWRESOLVERPRIVATE_H */
