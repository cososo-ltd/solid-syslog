#ifndef SOLIDSYSLOGPLUSTCPRESOLVERPRIVATE_H
#define SOLIDSYSLOGPLUSTCPRESOLVERPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPlusTcpResolverErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolverDefinition.h"

struct SolidSyslogPlusTcpResolver
{
    struct SolidSyslogResolver Base;
};

void PlusTcpResolver_Initialise(struct SolidSyslogResolver* base);
void PlusTcpResolver_Cleanup(struct SolidSyslogResolver* base);

static inline void PlusTcpResolver_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogPlusTcpResolverErrors code
)
{
    SolidSyslog_Error(severity, &PlusTcpResolverErrorSource, category, code);
}

#endif /* SOLIDSYSLOGPLUSTCPRESOLVERPRIVATE_H */
