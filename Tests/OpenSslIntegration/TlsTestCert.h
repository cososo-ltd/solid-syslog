#ifndef TLSTESTCERT_H
#define TLSTESTCERT_H

#include <time.h>
#include <openssl/types.h>

#include "ExternC.h"

EXTERN_C_BEGIN

    struct TlsTestCert
    {
        X509*     cert;
        EVP_PKEY* key;
    };

    struct TlsTestCertConfig
    {
        const char*               commonName;
        const char* const *       subjectAltDnsNames; /* NULL-terminated array; NULL if no SAN */
        time_t                    notBefore;          /* 0 = now */
        time_t                    notAfter;           /* 0 = now + 3600 */
        const struct TlsTestCert* issuer;             /* NULL = self-signed */
    };

    void TlsTestCert_Create(const struct TlsTestCertConfig* config, struct TlsTestCert* out);
    void TlsTestCert_Destroy(struct TlsTestCert * cert);
    void TlsTestCert_WritePemToFile(const struct TlsTestCert* cert, const char* path);
    void TlsTestCert_WritePrivateKeyPemToFile(const struct TlsTestCert* cert, const char* path);

EXTERN_C_END

#endif /* TLSTESTCERT_H */
