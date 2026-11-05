#ifndef WOLFSSLTLSFAKE_H
#define WOLFSSLTLSFAKE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogExternC.h"

struct WOLFSSL;
struct WOLFSSL_CTX;
struct WOLFSSL_METHOD;
struct WOLFSSL_X509;
struct WOLFSSL_X509_STORE_CTX;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Link-time double for the wolfSSL TLS API the WolfSslStream calls. Captures
       arguments and returns canned values; never negotiates anything. The
       context, method and session it hands out are dummies a test compares by
       address. Resets to the happy path: every call succeeds and the handshake
       completes first time. */
    void WolfSslTlsFake_Reset(void);

    /* wolfTLS_client_method_ex / wolfSSL_CTX_new_ex / wolfSSL_CTX_free */
    struct WOLFSSL_METHOD* WolfSslTlsFake_Method(void);
    int WolfSslTlsFake_ClientMethodExCallCount(void);
    void* WolfSslTlsFake_LastClientMethodExHeap(void);
    void WolfSslTlsFake_SetClientMethodExReturnsNull(bool returnsNull);
    struct WOLFSSL_CTX* WolfSslTlsFake_Ctx(void);
    int WolfSslTlsFake_CtxNewExCallCount(void);
    struct WOLFSSL_METHOD* WolfSslTlsFake_LastCtxNewExMethod(void);
    void* WolfSslTlsFake_LastCtxNewExHeap(void);
    void WolfSslTlsFake_SetCtxNewExReturnsNull(bool returnsNull);
    int WolfSslTlsFake_CtxFreeCallCount(void);
    struct WOLFSSL_CTX* WolfSslTlsFake_LastCtxFreeArg(void);

    /* wolfSSL_new / wolfSSL_free */
    struct WOLFSSL* WolfSslTlsFake_Session(void);
    int WolfSslTlsFake_NewCallCount(void);
    struct WOLFSSL_CTX* WolfSslTlsFake_LastNewCtx(void);
    void WolfSslTlsFake_SetNewReturnsNull(bool returnsNull);
    int WolfSslTlsFake_FreeCallCount(void);
    struct WOLFSSL* WolfSslTlsFake_LastFreeArg(void);
    /* How many sessions had been freed when the context was last freed. */
    int WolfSslTlsFake_FreesSeenAtCtxFree(void);

    /* wolfSSL_SetMinVersion and the key-size floors */
    int WolfSslTlsFake_SetMinVersionCallCount(void);
    struct WOLFSSL* WolfSslTlsFake_LastSetMinVersionSsl(void);
    int WolfSslTlsFake_LastSetMinVersionArg(void);
    void WolfSslTlsFake_SetSetMinVersionReturn(int value);
    int WolfSslTlsFake_LastMinRsaKeySz(void);
    void WolfSslTlsFake_SetSetMinRsaKeySzReturn(int value);
    int WolfSslTlsFake_LastMinEccKeySz(void);
    void WolfSslTlsFake_SetSetMinEccKeySzReturn(int value);
    int WolfSslTlsFake_LastMinDhKeySz(void);
    void WolfSslTlsFake_SetSetMinDhKeySzReturn(int value);
    /* How many sessions had been created when the floors were last set. */
    int WolfSslTlsFake_NewsSeenAtSetMinVersion(void);

    /* wolfSSL_set_cipher_list */
    int WolfSslTlsFake_SetCipherListCallCount(void);
    struct WOLFSSL* WolfSslTlsFake_LastSetCipherListSsl(void);
    const char* WolfSslTlsFake_LastSetCipherList(void);
    void WolfSslTlsFake_SetSetCipherListReturn(int value);

    /* wolfSSL_check_domain_name / wolfSSL_UseSNI */
    int WolfSslTlsFake_CheckDomainNameCallCount(void);
    struct WOLFSSL* WolfSslTlsFake_LastCheckDomainNameSsl(void);
    const char* WolfSslTlsFake_LastCheckDomainName(void);
    void WolfSslTlsFake_SetCheckDomainNameReturn(int value);
    int WolfSslTlsFake_CheckIpAddressCallCount(void);
    struct WOLFSSL* WolfSslTlsFake_LastCheckIpAddressSsl(void);
    const char* WolfSslTlsFake_LastCheckIpAddress(void);
    void WolfSslTlsFake_SetCheckIpAddressReturn(int value);
    int WolfSslTlsFake_UseSniCallCount(void);
    struct WOLFSSL* WolfSslTlsFake_LastUseSniSsl(void);
    unsigned char WolfSslTlsFake_LastUseSniType(void);
    const void* WolfSslTlsFake_LastUseSniData(void);
    unsigned short WolfSslTlsFake_LastUseSniSize(void);
    void WolfSslTlsFake_SetUseSniReturn(int value);

    /* wolfSSL_SSLSetIORecv / wolfSSL_SSLSetIOSend / wolfSSL_SetIOReadCtx /
       wolfSSL_SetIOWriteCtx */
    int (*WolfSslTlsFake_LastIoRecvCallback(void))(struct WOLFSSL*, char*, int, void*);
    int (*WolfSslTlsFake_LastIoSendCallback(void))(struct WOLFSSL*, char*, int, void*);
    void* WolfSslTlsFake_LastIoReadCtx(void);
    void* WolfSslTlsFake_LastIoWriteCtx(void);

    /* wolfSSL_set_verify / wolfSSL_SetCertCbCtx */
    int WolfSslTlsFake_SetVerifyCallCount(void);
    int WolfSslTlsFake_LastSetVerifyMode(void);
    int (*WolfSslTlsFake_LastSetVerifyCallback(void))(int, struct WOLFSSL_X509_STORE_CTX*);
    void* WolfSslTlsFake_LastCertCbCtx(void);
    /* A store context a test can hand the verify callback, carrying `error`
       for the certificate at `depth` - 0 the leaf, 1 its issuer. Its userCtx
       is whatever the stream last registered, and its certs the peer's chain
       below. */
    struct WOLFSSL_X509_STORE_CTX* WolfSslTlsFake_StoreWithError(int error);
    struct WOLFSSL_X509_STORE_CTX* WolfSslTlsFake_StoreAt(int error, int depth);
    /* The DER of each certificate in the chain the peer presents. */
    const unsigned char* WolfSslTlsFake_LeafDer(void);
    size_t WolfSslTlsFake_LeafDerLength(void);
    const unsigned char* WolfSslTlsFake_IssuerDer(void);
    size_t WolfSslTlsFake_IssuerDerLength(void);

    /* wolfSSL_connect, with wolfSSL_get_error answering for it. Each call takes
       the next entry of the sequence: 0 completes the handshake (WOLFSSL_SUCCESS)
       and any other value fails it with that value as the get_error answer -
       WOLFSSL_ERROR_WANT_READ, or a negative library code. Once exhausted, every
       later call repeats the last entry. */
    int WolfSslTlsFake_ConnectCallCount(void);
    struct WOLFSSL* WolfSslTlsFake_LastConnectArg(void);
    void WolfSslTlsFake_SetConnectErrorSequence(const int* errors, int count);
    /* Before wolfSSL_connect answers, the fake calls the registered verify
       callback once per entry with a store carrying that error, as the library
       does for each check the peer fails. An error of 0 is a certificate that
       verified, called back with preverify 1 as WOLFSSL_ALWAYS_VERIFY_CB does.
       Each entry is for the depth in the same place of SetVerifyDepths, or for
       the leaf where that has none. */
    void WolfSslTlsFake_SetVerifyErrors(const int* errors, int count);
    void WolfSslTlsFake_SetVerifyDepths(const int* depths, int count);
    int WolfSslTlsFake_LastVerifyCallbackResult(void);
    /* How many of those calls the stream answered by accepting the certificate. */
    int WolfSslTlsFake_VerifyCallbackAcceptCount(void);

    /* wolfSSL_write / wolfSSL_read, with wolfSSL_get_error answering for them.
       The return is what the call answers; the error is what get_error then
       says, read only where the return is not a positive count. */
    int WolfSslTlsFake_WriteCallCount(void);
    struct WOLFSSL* WolfSslTlsFake_LastWriteSsl(void);
    const void* WolfSslTlsFake_LastWriteData(void);
    int WolfSslTlsFake_LastWriteSz(void);
    void WolfSslTlsFake_SetWriteReturn(int value);
    int WolfSslTlsFake_ReadCallCount(void);
    struct WOLFSSL* WolfSslTlsFake_LastReadSsl(void);
    void* WolfSslTlsFake_LastReadData(void);
    int WolfSslTlsFake_LastReadSz(void);
    void WolfSslTlsFake_SetReadReturn(int value, int error);
    /* Per-call read returns, for a Send that reads before it writes. Exhausted,
       every later call repeats the last entry. */
    void WolfSslTlsFake_SetReadReturnSequence(const int* values, int count, int error);

    /* wolfSSL_shutdown */
    int WolfSslTlsFake_ShutdownCallCount(void);
    struct WOLFSSL* WolfSslTlsFake_LastShutdownArg(void);

    /* wolfSSL_CTX_load_verify_buffer / wolfSSL_CTX_use_certificate_chain_buffer /
       wolfSSL_CTX_use_PrivateKey_buffer, for the PEM-buffer credentials. */
    int WolfSslTlsFake_LoadVerifyBufferCallCount(void);
    struct WOLFSSL_CTX* WolfSslTlsFake_LastLoadVerifyBufferCtx(void);
    const unsigned char* WolfSslTlsFake_LastLoadVerifyBufferIn(void);
    long WolfSslTlsFake_LastLoadVerifyBufferSz(void);
    int WolfSslTlsFake_LastLoadVerifyBufferFormat(void);
    void WolfSslTlsFake_SetLoadVerifyBufferReturn(int value);
    int WolfSslTlsFake_UseCertificateChainBufferCallCount(void);
    const unsigned char* WolfSslTlsFake_LastUseCertificateChainBufferIn(void);
    long WolfSslTlsFake_LastUseCertificateChainBufferSz(void);
    void WolfSslTlsFake_SetUseCertificateChainBufferReturn(int value);
    int WolfSslTlsFake_UsePrivateKeyBufferCallCount(void);
    const unsigned char* WolfSslTlsFake_LastUsePrivateKeyBufferIn(void);
    long WolfSslTlsFake_LastUsePrivateKeyBufferSz(void);
    int WolfSslTlsFake_LastUsePrivateKeyBufferFormat(void);
    void WolfSslTlsFake_SetUsePrivateKeyBufferReturn(int value);

    /* wolfSSL_get_peer_certificate / wolfSSL_X509_get_der / wolfSSL_X509_free,
       which a build with KEEP_PEER_CERT has. The certificate handed out is a
       dummy whose DER is the leaf's. */
    struct WOLFSSL_X509* WolfSslTlsFake_PeerCertificate(void);
    int WolfSslTlsFake_GetPeerCertificateCallCount(void);
    struct WOLFSSL* WolfSslTlsFake_LastGetPeerCertificateSsl(void);
    void WolfSslTlsFake_SetPeerCertificateReturnsNull(bool returnsNull);
    void WolfSslTlsFake_SetX509GetDerReturnsNull(bool returnsNull);
    int WolfSslTlsFake_X509FreeCallCount(void);
    struct WOLFSSL_X509* WolfSslTlsFake_LastX509FreeArg(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* WOLFSSLTLSFAKE_H */
