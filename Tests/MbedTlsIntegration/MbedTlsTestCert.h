#ifndef MBEDTLSTESTCERT_H
#define MBEDTLSTESTCERT_H

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct MbedTlsTestCert
    {
        mbedtls_pk_context Key;
        mbedtls_x509_crt Cert;
        char SubjectName[128];
    };

    struct MbedTlsTestCertConfig
    {
        const char* SubjectName; /* X.509 subject string, e.g. "CN=test-ca" */
        const char* SubjectAltDns; /* SAN dnsName; NULL = no SAN */
        int IsCa; /* 1 = mark BasicConstraints CA:TRUE */
        const struct MbedTlsTestCert* Issuer; /* NULL = self-signed */
        /* "YYYYMMDDHHMMSS", as mbedtls_x509write_crt_set_validity takes them.
         * NULL on either leaves the default window, which is open now and
         * stays open past any plausible run of these tests. */
        const char* ValidityFrom;
        const char* ValidityTo;
    };

    /* Build a fresh RSA-2048 key + cert pair. The cert is parsed back into
     * `out->Cert` and is immediately usable by mbedtls_ssl_conf_ca_chain
     * (CA) or mbedtls_ssl_conf_own_cert (server / mTLS client). */
    void MbedTlsTestCert_Create(
        const struct MbedTlsTestCertConfig* config,
        struct MbedTlsTestCert* out,
        mbedtls_ctr_drbg_context* rng
    );

    void MbedTlsTestCert_Destroy(struct MbedTlsTestCert * cert);

SOLIDSYSLOG_EXTERN_C_END

#endif /* MBEDTLSTESTCERT_H */
