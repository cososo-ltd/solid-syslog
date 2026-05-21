#ifndef SOLIDSYSLOGMBEDTLSSTREAM_H
#define SOLIDSYSLOGMBEDTLSSTREAM_H

#include "ExternC.h"
#include "SolidSyslogSleep.h"

struct SolidSyslogStream;

/* Forward declarations keep the public header free of any mbedTLS include.
 * Integrators include the relevant mbedTLS headers themselves before this
 * one to bring the types into scope. See project_mbedtls_di_handles. */
struct mbedtls_ctr_drbg_context;
struct mbedtls_x509_crt;
struct mbedtls_pk_context;

EXTERN_C_BEGIN

    struct SolidSyslogMbedTlsStreamConfig
    {
        struct SolidSyslogStream* Transport; /* underlying byte stream — caller owns */
        SolidSyslogSleepFunction
            Sleep; /* drives bounded handshake retry between WANT_READ/WANT_WRITE polls — required */
        struct mbedtls_ctr_drbg_context* Rng; /* seeded CTR-DRBG — caller owns */
        struct mbedtls_x509_crt* CaChain; /* trust anchors — caller owns */
        const char* ServerName; /* SNI + cert hostname check; NULL to skip */
        struct mbedtls_x509_crt* ClientCertChain; /* leaf (+ intermediates); NULL = no mTLS */
        struct mbedtls_pk_context* ClientKey; /* matching private key; NULL = no mTLS */
    };

    struct SolidSyslogStream* SolidSyslogMbedTlsStream_Create(const struct SolidSyslogMbedTlsStreamConfig* config);
    void SolidSyslogMbedTlsStream_Destroy(struct SolidSyslogStream * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGMBEDTLSSTREAM_H */
