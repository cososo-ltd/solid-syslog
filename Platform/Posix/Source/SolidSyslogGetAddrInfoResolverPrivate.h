#ifndef SOLIDSYSLOGGETADDRINFORESOLVERPRIVATE_H
#define SOLIDSYSLOGGETADDRINFORESOLVERPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogGetAddrInfoResolverErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolverDefinition.h"

struct SolidSyslogGetAddrInfoResolver
{
    struct SolidSyslogResolver Base;
};

void GetAddrInfoResolver_Initialise(struct SolidSyslogResolver* base);
void GetAddrInfoResolver_Cleanup(struct SolidSyslogResolver* base);

static inline void GetAddrInfoResolver_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogGetAddrInfoResolverErrors code
)
{
    SolidSyslog_Error(severity, &GetAddrInfoResolverErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGGETADDRINFORESOLVERPRIVATE_H */
