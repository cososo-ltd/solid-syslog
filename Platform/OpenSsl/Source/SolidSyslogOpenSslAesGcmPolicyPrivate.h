#ifndef SOLIDSYSLOGOPENSSLAESGCMPOLICYPRIVATE_H
#define SOLIDSYSLOGOPENSSLAESGCMPOLICYPRIVATE_H

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

/* Emits one error from this class's source — hides the source pointer and the
 * enum-to-uint8 cast from every call site (seal/open in Policy.c, the pool in
 * Static.c). */
void OpenSslAesGcmPolicy_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogOpenSslAesGcmPolicyErrors code
);

#endif /* SOLIDSYSLOGOPENSSLAESGCMPOLICYPRIVATE_H */
