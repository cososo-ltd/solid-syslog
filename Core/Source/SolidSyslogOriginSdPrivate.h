#ifndef SOLIDSYSLOGORIGINSDPRIVATE_H
#define SOLIDSYSLOGORIGINSDPRIVATE_H

#include "SolidSyslogFormatter.h"
#include "SolidSyslogOriginSd.h"
#include "SolidSyslogStructuredDataDefinition.h"

enum
{
    ORIGIN_SOFTWARE_MAX = 48,
    ORIGIN_SWVERSION_MAX = 32,
    ORIGIN_ENTERPRISE_ID_MAX = 64,
    ORIGIN_IP_MAX = 64,
    ORIGIN_LITERAL_BYTES =
        48, /* [origin software="" swVersion="" enterpriseId="" — closing ']' deferred to per-message OriginSd_Format */
    ORIGIN_CONTENT_MAX = ORIGIN_LITERAL_BYTES + SOLIDSYSLOG_ESCAPED_MAX_SIZE(ORIGIN_SOFTWARE_MAX) +
                         SOLIDSYSLOG_ESCAPED_MAX_SIZE(ORIGIN_SWVERSION_MAX) +
                         SOLIDSYSLOG_ESCAPED_MAX_SIZE(ORIGIN_ENTERPRISE_ID_MAX),
    ORIGIN_FORMATTED_MAX = ORIGIN_CONTENT_MAX + 1 /* null terminator */
};

struct SolidSyslogOriginSd
{
    struct SolidSyslogStructuredData Base;
    SolidSyslogOriginIpCountFunction GetIpCount;
    SolidSyslogOriginIpAtFunction GetIpAt;
    SolidSyslogFormatterStorage FormattedStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(ORIGIN_FORMATTED_MAX)];
};

void OriginSd_Initialise(struct SolidSyslogStructuredData* base, const struct SolidSyslogOriginSdConfig* config);
void OriginSd_Cleanup(struct SolidSyslogStructuredData* base);

#endif /* SOLIDSYSLOGORIGINSDPRIVATE_H */
