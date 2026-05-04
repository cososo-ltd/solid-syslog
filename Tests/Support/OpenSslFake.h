#ifndef OPENSSLFAKE_H
#define OPENSSLFAKE_H

#include "ExternC.h"

#include <stdbool.h> // IWYU pragma: keep — dual-use header (C and C++ TUs); IWYU only sees the C++ side and would drop this.

/* Forward-declared OpenSSL types — full definitions live in <openssl/ssl.h>. */
struct ssl_ctx_st;
struct ssl_st;
struct ssl_method_st;
struct bio_st;
struct bio_method_st;

EXTERN_C_BEGIN

    void OpenSslFake_Reset(void);

    /* Failure-mode switches */
    void OpenSslFake_SetConnectFails(bool fails);
    void OpenSslFake_SetWriteFails(bool fails);
    void OpenSslFake_SetSet1HostFails(bool fails);
    void OpenSslFake_SetSniHostnameFails(bool fails);
    void OpenSslFake_SetCtxNewFails(bool fails);
    void OpenSslFake_SetSslNewFails(bool fails);
    void OpenSslFake_SetLoadVerifyLocationsFails(bool fails);
    void OpenSslFake_SetMinProtoVersionFails(bool fails);
    void OpenSslFake_SetBioMethNewFails(bool fails);
    void OpenSslFake_SetBioNewFails(bool fails);
    void OpenSslFake_SetCipherListFails(bool fails);

    /* SSL_CTX_new */
    int                         OpenSslFake_CtxNewCallCount(void);
    const struct ssl_method_st* OpenSslFake_LastCtxNewMethodArg(void);
    struct ssl_ctx_st*          OpenSslFake_LastCtxReturned(void);

    /* SSL_CTX_load_verify_locations */
    struct ssl_ctx_st* OpenSslFake_LastLoadVerifyLocationsCtxArg(void);
    const char*        OpenSslFake_LastCaBundlePath(void);

    /* SSL_CTX_set_verify */
    struct ssl_ctx_st* OpenSslFake_LastSetVerifyCtxArg(void);
    int                OpenSslFake_LastVerifyMode(void);

    /* SSL_CTX_ctrl (SET_MIN_PROTO_VERSION path) */
    struct ssl_ctx_st* OpenSslFake_LastSslCtxCtrlCtxArg(void);
    long               OpenSslFake_LastMinProtoVersion(void);

    /* SSL_CTX_set_cipher_list */
    int                OpenSslFake_SetCipherListCallCount(void);
    struct ssl_ctx_st* OpenSslFake_LastSetCipherListCtxArg(void);
    const char*        OpenSslFake_LastCipherList(void);

    /* SSL_new */
    int                OpenSslFake_SslNewCallCount(void);
    struct ssl_ctx_st* OpenSslFake_LastSslNewCtxArg(void);
    struct ssl_st*     OpenSslFake_LastSslReturned(void);

    /* BIO_meth_new */
    struct bio_method_st* OpenSslFake_LastBioMethReturned(void);

    /* BIO_meth_free */
    int                   OpenSslFake_BioMethFreeCallCount(void);
    struct bio_method_st* OpenSslFake_LastBioMethFreeArg(void);

    /* BIO_meth_set_read */
    struct bio_method_st* OpenSslFake_LastBioMethSetReadMethodArg(void);
    int (*OpenSslFake_LastBioReadCallback(void))(struct bio_st*, char*, int);
    long (*OpenSslFake_LastBioCtrlCallback(void))(struct bio_st*, int, long, void*);
    int (*OpenSslFake_LastBioCreateCallback(void))(struct bio_st*);
    int OpenSslFake_LastSetInitArg(void);

    /* BIO_meth_set_write */
    struct bio_method_st* OpenSslFake_LastBioMethSetWriteMethodArg(void);
    int (*OpenSslFake_LastBioWriteCallback(void))(struct bio_st*, const char*, int);

    /* BIO_new */
    int                         OpenSslFake_BioNewCallCount(void);
    const struct bio_method_st* OpenSslFake_LastBioNewMethodArg(void);
    struct bio_st*              OpenSslFake_LastBioReturned(void);

    /* BIO_set_data */
    struct bio_st* OpenSslFake_LastSetDataBioArg(void);
    void*          OpenSslFake_LastSetDataArg(void);

    /* BIO_get_data (arg capture for BIO-callback introspection) */
    struct bio_st* OpenSslFake_LastGetDataBioArg(void);

    /* SSL_set_bio */
    int            OpenSslFake_SetBioCallCount(void);
    struct ssl_st* OpenSslFake_LastSetBioSslArg(void);
    struct bio_st* OpenSslFake_LastSetBioReadBioArg(void);
    struct bio_st* OpenSslFake_LastSetBioWriteBioArg(void);

    /* SSL_ctrl (SET_TLSEXT_HOSTNAME path — SNI) */
    struct ssl_st* OpenSslFake_LastSslCtrlSslArg(void);
    const char*    OpenSslFake_LastSniHostname(void);

    /* SSL_set1_host */
    struct ssl_st* OpenSslFake_LastSet1HostSslArg(void);
    const char*    OpenSslFake_LastSet1Host(void);

    /* SSL_connect */
    int            OpenSslFake_ConnectCallCount(void);
    struct ssl_st* OpenSslFake_LastConnectSslArg(void);

    /* SSL_write */
    int            OpenSslFake_WriteCallCount(void);
    struct ssl_st* OpenSslFake_LastWriteSslArg(void);
    const void*    OpenSslFake_LastWriteBuf(void);
    int            OpenSslFake_LastWriteSize(void);

    /* SSL_read */
    int            OpenSslFake_SslReadCallCount(void);
    struct ssl_st* OpenSslFake_LastSslReadSslArg(void);
    void*          OpenSslFake_LastSslReadBuf(void);
    int            OpenSslFake_LastSslReadSize(void);

    /* SSL_shutdown */
    int            OpenSslFake_ShutdownCallCount(void);
    struct ssl_st* OpenSslFake_LastShutdownSslArg(void);

    /* SSL_free */
    int            OpenSslFake_FreeCallCount(void);
    struct ssl_st* OpenSslFake_LastFreeSslArg(void);

    /* SSL_CTX_free */
    int                OpenSslFake_CtxFreeCallCount(void);
    struct ssl_ctx_st* OpenSslFake_LastCtxFreeCtxArg(void);

    /* SSL_CTX_use_certificate_chain_file */
    int                OpenSslFake_UseCertChainFileCallCount(void);
    struct ssl_ctx_st* OpenSslFake_LastUseCertChainFileCtxArg(void);
    const char*        OpenSslFake_LastClientCertChainPath(void);
    void               OpenSslFake_SetUseCertChainFileFails(bool fails);

    /* SSL_CTX_use_PrivateKey_file */
    int                OpenSslFake_UsePrivateKeyFileCallCount(void);
    struct ssl_ctx_st* OpenSslFake_LastUsePrivateKeyFileCtxArg(void);
    const char*        OpenSslFake_LastClientKeyPath(void);
    int                OpenSslFake_LastClientKeyFileType(void);
    void               OpenSslFake_SetUsePrivateKeyFileFails(bool fails);

    /* SSL_CTX_check_private_key */
    int                OpenSslFake_CheckPrivateKeyCallCount(void);
    struct ssl_ctx_st* OpenSslFake_LastCheckPrivateKeyCtxArg(void);
    void               OpenSslFake_SetCheckPrivateKeyFails(bool fails);

EXTERN_C_END

#endif /* OPENSSLFAKE_H */
