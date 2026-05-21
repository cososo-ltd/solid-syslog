#include "MbedTlsTestCert.h"

#include <mbedtls/asn1.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/oid.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <stddef.h>
#include <string.h>

enum
{
    RSA_KEY_BITS = 2048,
    RSA_EXPONENT = 65537,
    DER_BUFFER_BYTES = 4096
};

static void GenerateKey(mbedtls_pk_context* key, mbedtls_ctr_drbg_context* rng);
static void WriteCertToDer(
    const struct MbedTlsTestCertConfig* config,
    mbedtls_pk_context* subjectKey,
    unsigned char* derBuffer,
    size_t bufferSize,
    size_t* derStartOffset,
    size_t* derLength,
    mbedtls_ctr_drbg_context* rng
);

void MbedTlsTestCert_Create(
    const struct MbedTlsTestCertConfig* config,
    struct MbedTlsTestCert* out,
    mbedtls_ctr_drbg_context* rng
)
{
    mbedtls_pk_init(&out->Key);
    mbedtls_x509_crt_init(&out->Cert);
    strncpy(out->SubjectName, config->SubjectName, sizeof(out->SubjectName) - 1U);
    out->SubjectName[sizeof(out->SubjectName) - 1U] = '\0';

    GenerateKey(&out->Key, rng);

    unsigned char derBuffer[DER_BUFFER_BYTES];
    size_t derStartOffset = 0U;
    size_t derLength = 0U;
    WriteCertToDer(config, &out->Key, derBuffer, sizeof(derBuffer), &derStartOffset, &derLength, rng);

    mbedtls_x509_crt_parse_der(&out->Cert, &derBuffer[derStartOffset], derLength);
}

void MbedTlsTestCert_Destroy(struct MbedTlsTestCert* cert)
{
    mbedtls_x509_crt_free(&cert->Cert);
    mbedtls_pk_free(&cert->Key);
}

static void GenerateKey(mbedtls_pk_context* key, mbedtls_ctr_drbg_context* rng)
{
    mbedtls_pk_setup(key, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    mbedtls_rsa_gen_key(mbedtls_pk_rsa(*key), mbedtls_ctr_drbg_random, rng, RSA_KEY_BITS, RSA_EXPONENT);
}

static void WriteCertToDer(
    const struct MbedTlsTestCertConfig* config,
    mbedtls_pk_context* subjectKey,
    unsigned char* derBuffer,
    size_t bufferSize,
    size_t* derStartOffset,
    size_t* derLength,
    mbedtls_ctr_drbg_context* rng
)
{
    mbedtls_x509write_cert crt;
    mbedtls_x509write_crt_init(&crt);
    mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
    mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
    mbedtls_x509write_crt_set_subject_key(&crt, subjectKey);
    mbedtls_x509write_crt_set_subject_name(&crt, config->SubjectName);

    /* Self-signed when Issuer == NULL — issuer name + key match the subject. */
    const char* issuerName = (config->Issuer != NULL) ? config->Issuer->SubjectName : config->SubjectName;
    mbedtls_pk_context* issuerKey = (config->Issuer != NULL) ? (mbedtls_pk_context*) &config->Issuer->Key : subjectKey;
    mbedtls_x509write_crt_set_issuer_name(&crt, issuerName);
    mbedtls_x509write_crt_set_issuer_key(&crt, issuerKey);

    const unsigned char serial[] = {0x01};
    mbedtls_x509write_crt_set_serial_raw(&crt, (unsigned char*) serial, sizeof(serial));

    /* Long-validity test certs: 2024-01-01 to 2099-01-01. */
    mbedtls_x509write_crt_set_validity(&crt, "20240101000000", "20990101000000");

    mbedtls_x509write_crt_set_basic_constraints(&crt, config->IsCa, -1);

    mbedtls_x509_san_list san;
    if (config->SubjectAltDns != NULL)
    {
        memset(&san, 0, sizeof(san));
        san.node.type = MBEDTLS_X509_SAN_DNS_NAME;
        san.node.san.unstructured_name.tag = MBEDTLS_ASN1_IA5_STRING;
        san.node.san.unstructured_name.p = (unsigned char*) config->SubjectAltDns;
        san.node.san.unstructured_name.len = strlen(config->SubjectAltDns);
        san.next = NULL;
        mbedtls_x509write_crt_set_subject_alternative_name(&crt, &san);
    }

    /* mbedtls_x509write_crt_der writes at the END of the buffer, returning
     * the number of bytes written. The actual DER starts at offset
     * (bufferSize - written). */
    int written = mbedtls_x509write_crt_der(&crt, derBuffer, bufferSize, mbedtls_ctr_drbg_random, rng);
    if (written > 0)
    {
        *derStartOffset = bufferSize - (size_t) written;
        *derLength = (size_t) written;
    }

    mbedtls_x509write_crt_free(&crt);
}
