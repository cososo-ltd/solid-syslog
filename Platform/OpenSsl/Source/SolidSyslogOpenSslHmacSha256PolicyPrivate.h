#ifndef SOLIDSYSLOGOPENSSLHMACSHA256POLICYPRIVATE_H
#define SOLIDSYSLOGOPENSSLHMACSHA256POLICYPRIVATE_H

#include "SolidSyslogOpenSslHmacSha256Policy.h"
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

#endif /* SOLIDSYSLOGOPENSSLHMACSHA256POLICYPRIVATE_H */
