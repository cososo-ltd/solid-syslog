#ifndef SOLIDSYSLOGFREERTOSRESOLVERPRIVATE_H
#define SOLIDSYSLOGFREERTOSRESOLVERPRIVATE_H

#include <stdint.h>

#include "SolidSyslogResolverDefinition.h"

struct SolidSyslogFreeRtosResolver
{
    struct SolidSyslogResolver Base;
    uint8_t Octets[4];
};

void FreeRtosResolver_Initialise(struct SolidSyslogResolver* base, const uint8_t ipv4Octets[4]);
void FreeRtosResolver_Cleanup(struct SolidSyslogResolver* base);

#endif /* SOLIDSYSLOGFREERTOSRESOLVERPRIVATE_H */
