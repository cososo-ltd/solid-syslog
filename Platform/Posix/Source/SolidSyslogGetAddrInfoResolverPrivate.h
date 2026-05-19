#ifndef SOLIDSYSLOGGETADDRINFORESOLVERPRIVATE_H
#define SOLIDSYSLOGGETADDRINFORESOLVERPRIVATE_H

#include "SolidSyslogResolverDefinition.h"

struct SolidSyslogGetAddrInfoResolver
{
    struct SolidSyslogResolver Base;
};

void GetAddrInfoResolver_Initialise(struct SolidSyslogResolver* base);
void GetAddrInfoResolver_Cleanup(struct SolidSyslogResolver* base);

#endif /* SOLIDSYSLOGGETADDRINFORESOLVERPRIVATE_H */
