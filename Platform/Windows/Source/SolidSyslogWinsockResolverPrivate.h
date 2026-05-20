#ifndef SOLIDSYSLOGWINSOCKRESOLVERPRIVATE_H
#define SOLIDSYSLOGWINSOCKRESOLVERPRIVATE_H

#include "SolidSyslogResolverDefinition.h"

struct SolidSyslogWinsockResolver
{
    struct SolidSyslogResolver Base;
};

void WinsockResolver_Initialise(struct SolidSyslogResolver* base);
void WinsockResolver_Cleanup(struct SolidSyslogResolver* base);

#endif /* SOLIDSYSLOGWINSOCKRESOLVERPRIVATE_H */
