#ifndef SOLIDSYSLOGMBEDTLSHMACSHA256POLICYPRIVATE_H
#define SOLIDSYSLOGMBEDTLSHMACSHA256POLICYPRIVATE_H

#include "SolidSyslogMbedTlsHmacSha256Policy.h"
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

#endif /* SOLIDSYSLOGMBEDTLSHMACSHA256POLICYPRIVATE_H */
