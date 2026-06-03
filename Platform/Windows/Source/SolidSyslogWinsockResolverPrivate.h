#ifndef SOLIDSYSLOGWINSOCKRESOLVERPRIVATE_H
#define SOLIDSYSLOGWINSOCKRESOLVERPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogWinsockResolverErrors.h"

struct SolidSyslogWinsockResolver
{
    struct SolidSyslogResolver Base;
};

void WinsockResolver_Initialise(struct SolidSyslogResolver* base);
void WinsockResolver_Cleanup(struct SolidSyslogResolver* base);

static inline void WinsockResolver_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogWinsockResolverErrors code
)
{
    SolidSyslog_Error(severity, &WinsockResolverErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGWINSOCKRESOLVERPRIVATE_H */
