#ifndef SOLIDSYSLOGMBEDTLSAESGCMPOLICYPRIVATE_H
#define SOLIDSYSLOGMBEDTLSAESGCMPOLICYPRIVATE_H

#include "SolidSyslogMbedTlsAesGcmPolicy.h"
#include "SolidSyslogMbedTlsAesGcmPolicyErrors.h"
#include "SolidSyslogPrival.h"
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

/* Emits one error from this class's source — hides the source pointer and the
 * enum-to-uint8 cast from every call site (seal/open in Policy.c, the pool in
 * Static.c). */
void MbedTlsAesGcmPolicy_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogMbedTlsAesGcmPolicyErrors code
);

#endif /* SOLIDSYSLOGMBEDTLSAESGCMPOLICYPRIVATE_H */
