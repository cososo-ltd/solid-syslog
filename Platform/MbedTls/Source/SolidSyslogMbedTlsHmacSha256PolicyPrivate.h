#ifndef SOLIDSYSLOGMBEDTLSHMACSHA256POLICYPRIVATE_H
#define SOLIDSYSLOGMBEDTLSHMACSHA256POLICYPRIVATE_H

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

/* Emits one error from this class's source — hides the source pointer and the
 * enum-to-uint8 cast from every call site (seal/verify in Policy.c, the pool in
 * Static.c). */
void MbedTlsHmacSha256Policy_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogMbedTlsHmacSha256PolicyErrors code
);

#endif /* SOLIDSYSLOGMBEDTLSHMACSHA256POLICYPRIVATE_H */
