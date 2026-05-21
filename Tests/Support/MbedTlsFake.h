#ifndef MBEDTLSFAKE_H
#define MBEDTLSFAKE_H

#include <stddef.h>

#include "ExternC.h"

struct mbedtls_ssl_config;
struct mbedtls_ssl_context;
struct mbedtls_x509_crt;
struct mbedtls_x509_crl;
struct mbedtls_ctr_drbg_context;

EXTERN_C_BEGIN

    /* Resets every counter and captured-arg field to its initial state.
     * Call from TEST_GROUP::setup() so each test starts from a clean slate. */
    void MbedTlsFake_Reset(void);

    /* mbedtls_ssl_config_init */
    int MbedTlsFake_SslConfigInitCallCount(void);
    struct mbedtls_ssl_config* MbedTlsFake_LastSslConfigInitArg(void);

    /* mbedtls_ssl_config_defaults */
    int MbedTlsFake_SslConfigDefaultsCallCount(void);
    struct mbedtls_ssl_config* MbedTlsFake_LastSslConfigDefaultsConfigArg(void);
    int MbedTlsFake_LastSslConfigDefaultsEndpoint(void);
    int MbedTlsFake_LastSslConfigDefaultsTransport(void);
    int MbedTlsFake_LastSslConfigDefaultsPreset(void);

    /* mbedtls_ssl_init */
    int MbedTlsFake_SslInitCallCount(void);
    struct mbedtls_ssl_context* MbedTlsFake_LastSslInitArg(void);

    /* mbedtls_ssl_setup */
    int MbedTlsFake_SslSetupCallCount(void);
    struct mbedtls_ssl_context* MbedTlsFake_LastSslSetupContextArg(void);
    const struct mbedtls_ssl_config* MbedTlsFake_LastSslSetupConfigArg(void);

    /* mbedtls_ssl_set_bio */
    int MbedTlsFake_SslSetBioCallCount(void);
    struct mbedtls_ssl_context* MbedTlsFake_LastSslSetBioContextArg(void);
    void* MbedTlsFake_LastSslSetBioPBioArg(void);
    int (*MbedTlsFake_LastSslSetBioSendCallback(void))(void*, const unsigned char*, size_t);
    int (*MbedTlsFake_LastSslSetBioRecvCallback(void))(void*, unsigned char*, size_t);

    /* mbedtls_ssl_handshake */
    int MbedTlsFake_SslHandshakeCallCount(void);
    struct mbedtls_ssl_context* MbedTlsFake_LastSslHandshakeArg(void);
    void MbedTlsFake_SetSslHandshakeReturn(int value);

    /* mbedtls_ssl_write */
    int MbedTlsFake_SslWriteCallCount(void);
    struct mbedtls_ssl_context* MbedTlsFake_LastSslWriteContextArg(void);
    const unsigned char* MbedTlsFake_LastSslWriteBufArg(void);
    size_t MbedTlsFake_LastSslWriteLenArg(void);
    void MbedTlsFake_SetSslWriteReturn(int value);

    /* mbedtls_ssl_read */
    int MbedTlsFake_SslReadCallCount(void);
    struct mbedtls_ssl_context* MbedTlsFake_LastSslReadContextArg(void);
    unsigned char* MbedTlsFake_LastSslReadBufArg(void);
    size_t MbedTlsFake_LastSslReadLenArg(void);
    void MbedTlsFake_SetSslReadReturn(int value);

    /* mbedtls_ssl_close_notify */
    int MbedTlsFake_SslCloseNotifyCallCount(void);
    struct mbedtls_ssl_context* MbedTlsFake_LastSslCloseNotifyArg(void);

    /* mbedtls_ssl_free */
    int MbedTlsFake_SslFreeCallCount(void);
    struct mbedtls_ssl_context* MbedTlsFake_LastSslFreeArg(void);

    /* mbedtls_ssl_config_free */
    int MbedTlsFake_SslConfigFreeCallCount(void);
    struct mbedtls_ssl_config* MbedTlsFake_LastSslConfigFreeArg(void);

    /* mbedtls_ssl_conf_authmode */
    int MbedTlsFake_SslConfAuthmodeCallCount(void);
    struct mbedtls_ssl_config* MbedTlsFake_LastSslConfAuthmodeConfigArg(void);
    int MbedTlsFake_LastSslConfAuthmodeArg(void);

    /* mbedtls_ssl_conf_ca_chain */
    int MbedTlsFake_SslConfCaChainCallCount(void);
    struct mbedtls_ssl_config* MbedTlsFake_LastSslConfCaChainConfigArg(void);
    struct mbedtls_x509_crt* MbedTlsFake_LastSslConfCaChainArg(void);
    struct mbedtls_x509_crl* MbedTlsFake_LastSslConfCaChainCrlArg(void);

    /* mbedtls_ssl_conf_rng */
    int MbedTlsFake_SslConfRngCallCount(void);
    struct mbedtls_ssl_config* MbedTlsFake_LastSslConfRngConfigArg(void);
    int (*MbedTlsFake_LastSslConfRngFuncArg(void))(void*, unsigned char*, size_t);
    void* MbedTlsFake_LastSslConfRngContextArg(void);

    /* mbedtls_ssl_set_hostname */
    int MbedTlsFake_SslSetHostnameCallCount(void);
    struct mbedtls_ssl_context* MbedTlsFake_LastSslSetHostnameContextArg(void);
    const char* MbedTlsFake_LastSslSetHostnameNameArg(void);

EXTERN_C_END

#endif /* MBEDTLSFAKE_H */
