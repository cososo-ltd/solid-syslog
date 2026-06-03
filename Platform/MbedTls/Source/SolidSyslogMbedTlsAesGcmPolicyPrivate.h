#ifndef SOLIDSYSLOGMBEDTLSAESGCMPOLICYPRIVATE_H
#define SOLIDSYSLOGMBEDTLSAESGCMPOLICYPRIVATE_H

#include "SolidSyslogMbedTlsAesGcmPolicy.h"
#include "SolidSyslogSecurityPolicyDefinition.h"

struct SolidSyslogMbedTlsAesGcmPolicy
{
    struct SolidSyslogSecurityPolicy Base;
    struct SolidSyslogMbedTlsAesGcmPolicyConfig Config;
};

void MbedTlsAesGcmPolicy_Initialise(
    struct SolidSyslogSecurityPolicy* base,
    const struct SolidSyslogMbedTlsAesGcmPolicyConfig* config
);
void MbedTlsAesGcmPolicy_Cleanup(struct SolidSyslogSecurityPolicy* base);

#endif /* SOLIDSYSLOGMBEDTLSAESGCMPOLICYPRIVATE_H */
