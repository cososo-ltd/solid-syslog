#ifndef SOLIDSYSLOGMBEDTLSHMACSHA256POLICYPRIVATE_H
#define SOLIDSYSLOGMBEDTLSHMACSHA256POLICYPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogMbedTlsHmacSha256Policy.h"
#include "SolidSyslogMbedTlsHmacSha256PolicyErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyDefinition.h"

struct SolidSyslogMbedTlsHmacSha256Policy
{
    struct SolidSyslogSecurityPolicy Base;
    struct SolidSyslogMbedTlsHmacSha256PolicyConfig Config;
};

void MbedTlsHmacSha256Policy_Initialise(
    struct SolidSyslogSecurityPolicy* base,
    const struct SolidSyslogMbedTlsHmacSha256PolicyConfig* config
);
void MbedTlsHmacSha256Policy_Cleanup(struct SolidSyslogSecurityPolicy* base);

static inline void MbedTlsHmacSha256Policy_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogMbedTlsHmacSha256PolicyErrors code
)
{
    SolidSyslog_Error(severity, &MbedTlsHmacSha256PolicyErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGMBEDTLSHMACSHA256POLICYPRIVATE_H */
