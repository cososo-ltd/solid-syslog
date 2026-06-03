#ifndef SOLIDSYSLOGOPENSSLAESGCMPOLICYPRIVATE_H
#define SOLIDSYSLOGOPENSSLAESGCMPOLICYPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogOpenSslAesGcmPolicy.h"
#include "SolidSyslogOpenSslAesGcmPolicyErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyDefinition.h"

struct SolidSyslogOpenSslAesGcmPolicy
{
    struct SolidSyslogSecurityPolicy Base;
    struct SolidSyslogOpenSslAesGcmPolicyConfig Config;
};

void OpenSslAesGcmPolicy_Initialise(
    struct SolidSyslogSecurityPolicy* base,
    const struct SolidSyslogOpenSslAesGcmPolicyConfig* config
);
void OpenSslAesGcmPolicy_Cleanup(struct SolidSyslogSecurityPolicy* base);

static inline void OpenSslAesGcmPolicy_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogOpenSslAesGcmPolicyErrors code
)
{
    SolidSyslog_Error(severity, &OpenSslAesGcmPolicyErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGOPENSSLAESGCMPOLICYPRIVATE_H */
