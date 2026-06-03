#ifndef SOLIDSYSLOGOPENSSLAESGCMPOLICYPRIVATE_H
#define SOLIDSYSLOGOPENSSLAESGCMPOLICYPRIVATE_H

#include "SolidSyslogOpenSslAesGcmPolicy.h"
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

#endif /* SOLIDSYSLOGOPENSSLAESGCMPOLICYPRIVATE_H */
