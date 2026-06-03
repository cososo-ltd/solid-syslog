#ifndef SOLIDSYSLOGOPENSSLHMACSHA256POLICYPRIVATE_H
#define SOLIDSYSLOGOPENSSLHMACSHA256POLICYPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogOpenSslHmacSha256Policy.h"
#include "SolidSyslogOpenSslHmacSha256PolicyErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyDefinition.h"

struct SolidSyslogOpenSslHmacSha256Policy
{
    struct SolidSyslogSecurityPolicy Base;
    struct SolidSyslogOpenSslHmacSha256PolicyConfig Config;
};

void OpenSslHmacSha256Policy_Initialise(
    struct SolidSyslogSecurityPolicy* base,
    const struct SolidSyslogOpenSslHmacSha256PolicyConfig* config
);
void OpenSslHmacSha256Policy_Cleanup(struct SolidSyslogSecurityPolicy* base);

static inline void OpenSslHmacSha256Policy_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogOpenSslHmacSha256PolicyErrors code
)
{
    SolidSyslog_Error(severity, &OpenSslHmacSha256PolicyErrorSource, category, code);
}

#endif /* SOLIDSYSLOGOPENSSLHMACSHA256POLICYPRIVATE_H */
