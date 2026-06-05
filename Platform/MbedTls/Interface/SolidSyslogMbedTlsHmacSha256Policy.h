#ifndef SOLIDSYSLOGMBEDTLSHMACSHA256POLICY_H
#define SOLIDSYSLOGMBEDTLSHMACSHA256POLICY_H

#include "ExternC.h"
#include "SolidSyslogKeyFunction.h"

struct SolidSyslogSecurityPolicy;

EXTERN_C_BEGIN

    struct SolidSyslogMbedTlsHmacSha256PolicyConfig
    {
        SolidSyslogKeyFunction GetKey; /* fetches the HMAC key on demand — required; must report at
                                          least 32 bytes (SHA-256 output size) or seal/verify fails closed */
        void* KeyContext; /* passed through to GetKey; NULL is fine */
    };

    struct SolidSyslogSecurityPolicy* SolidSyslogMbedTlsHmacSha256Policy_Create(
        const struct SolidSyslogMbedTlsHmacSha256PolicyConfig* config
    );
    void SolidSyslogMbedTlsHmacSha256Policy_Destroy(struct SolidSyslogSecurityPolicy * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGMBEDTLSHMACSHA256POLICY_H */
