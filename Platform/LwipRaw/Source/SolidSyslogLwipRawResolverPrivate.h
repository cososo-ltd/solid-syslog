#ifndef SOLIDSYSLOGLWIPRAWRESOLVERPRIVATE_H
#define SOLIDSYSLOGLWIPRAWRESOLVERPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipRawResolverErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolverDefinition.h"

struct SolidSyslogLwipRawResolver
{
    struct SolidSyslogResolver Base;
};

void LwipRawResolver_Initialise(struct SolidSyslogResolver* base);
void LwipRawResolver_Cleanup(struct SolidSyslogResolver* base);

static inline void LwipRawResolver_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogLwipRawResolverErrors code
)
{
    SolidSyslog_Error(severity, &LwipRawResolverErrorSource, category, code);
}

#endif /* SOLIDSYSLOGLWIPRAWRESOLVERPRIVATE_H */
