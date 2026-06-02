#ifndef SOLIDSYSLOGOPENSSLHMACSHA256POLICYPRIVATE_H
#define SOLIDSYSLOGOPENSSLHMACSHA256POLICYPRIVATE_H

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

/* Emits one error from this class's source — hides the source pointer and the
 * enum-to-uint8 cast from every call site (seal/verify in Policy.c, the pool in
 * Static.c). */
void OpenSslHmacSha256Policy_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogOpenSslHmacSha256PolicyErrors code
);

#endif /* SOLIDSYSLOGOPENSSLHMACSHA256POLICYPRIVATE_H */
