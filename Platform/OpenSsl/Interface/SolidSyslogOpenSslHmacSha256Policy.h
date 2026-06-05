#ifndef SOLIDSYSLOGOPENSSLHMACSHA256POLICY_H
#define SOLIDSYSLOGOPENSSLHMACSHA256POLICY_H

#include "ExternC.h"
#include "SolidSyslogKeyFunction.h"

struct SolidSyslogSecurityPolicy;

EXTERN_C_BEGIN

    struct SolidSyslogOpenSslHmacSha256PolicyConfig
    {
        SolidSyslogKeyFunction GetKey; /* fetches the HMAC key on demand — required; must report at
                                          least 32 bytes (SHA-256 output size) or seal/verify fails closed */
        void* KeyContext; /* passed through to GetKey; NULL is fine */
    };

    struct SolidSyslogSecurityPolicy* SolidSyslogOpenSslHmacSha256Policy_Create(
        const struct SolidSyslogOpenSslHmacSha256PolicyConfig* config
    );
    void SolidSyslogOpenSslHmacSha256Policy_Destroy(struct SolidSyslogSecurityPolicy * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGOPENSSLHMACSHA256POLICY_H */
