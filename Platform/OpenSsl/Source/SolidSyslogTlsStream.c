#include "SolidSyslogTlsStream.h"

#include <openssl/bio.h>
#include <openssl/prov_ssl.h>
#include <openssl/ssl.h>
#include <openssl/types.h>
#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogNullStream.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTlsStreamPrivate.h"

enum
{
    /* Bounded retry budget for the TLS handshake under non-blocking transport.
       Real-world handshakes take ~100–500 ms (1–3 RTTs to a cloud SIEM); 5 s
       comfortably covers WAN deployments without burning the service thread
       indefinitely on a wedged peer. */
    HANDSHAKE_TIMEOUT_MILLISECONDS = 5000,
    HANDSHAKE_POLL_INTERVAL_MILLISECONDS = 1
};

struct SolidSyslogAddress;

static inline struct SolidSyslogTlsStream* TlsStream_SelfFromBase(struct SolidSyslogStream* base);

static inline bool TlsStream_AttachTransportBio(struct SolidSyslogTlsStream* self);
static inline void TlsStream_Close(struct SolidSyslogStream* base);
static inline bool TlsStream_ConfigureCipherList(SSL_CTX* ctx, const char* cipherList);
static inline bool TlsStream_ConfigureClientIdentity(SSL_CTX* ctx, const struct SolidSyslogTlsStreamConfig* config);
static inline bool TlsStream_ConfigureExpectedHostname(struct SolidSyslogTlsStream* self);
static inline bool TlsStream_ConfigureProtocolFloor(SSL_CTX* ctx);
static inline bool TlsStream_ConfigureSslContext(SSL_CTX* ctx, const struct SolidSyslogTlsStreamConfig* config);
static inline bool TlsStream_ConfigureTrustAnchors(SSL_CTX* ctx, const char* caBundlePath);
static inline SSL_CTX* TlsStream_CreateSslContext(const struct SolidSyslogTlsStreamConfig* config);
static inline BIO* TlsStream_CreateTransportBio(struct SolidSyslogTlsStream* self);
static inline BIO_METHOD* TlsStream_CreateTransportBioMethod(void);
static inline bool TlsStream_InitSslContext(struct SolidSyslogTlsStream* self);
static inline bool TlsStream_InitSslSession(struct SolidSyslogTlsStream* self);
static inline bool TlsStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);
static inline bool TlsStream_PerformHandshake(struct SolidSyslogTlsStream* self);
static inline SolidSyslogSsize TlsStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size);
static inline void TlsStream_ReleaseBioMethod(struct SolidSyslogTlsStream* self);
static inline void TlsStream_ReleaseHandshakeState(struct SolidSyslogTlsStream* self);
static inline void TlsStream_ReleaseSsl(struct SolidSyslogTlsStream* self);
static inline void TlsStream_ReleaseSslContext(struct SolidSyslogTlsStream* self);
static inline bool TlsStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size);
static inline int TlsStream_TransportBioCreate(BIO* bio);
static inline long TlsStream_TransportBioCtrl(BIO* bio, int cmd, long larg, void* parg);
static inline int TlsStream_TransportBioRead(BIO* bio, char* buffer, int size);
static inline int TlsStream_TransportBioWrite(BIO* bio, const char* buffer, int size);

void TlsStream_Initialise(struct SolidSyslogStream* base, const struct SolidSyslogTlsStreamConfig* config)
{
    struct SolidSyslogTlsStream* self = TlsStream_SelfFromBase(base);
    self->Base.Open = TlsStream_Open;
    self->Base.Send = TlsStream_Send;
    self->Base.Read = TlsStream_Read;
    self->Base.Close = TlsStream_Close;
    self->Config = *config;
    self->Ctx = NULL;
    self->Ssl = NULL;
    self->BioMethod = NULL;
}

void TlsStream_Cleanup(struct SolidSyslogStream* base)
{
    struct SolidSyslogTlsStream* self = TlsStream_SelfFromBase(base);
    /* Close first so an integrator who destroys a still-Open stream doesn't
     * leak the underlying transport. Close is idempotent (guards on Ssl !=
     * NULL for the TLS-side teardown; transport Close is itself idempotent
     * on every Stream impl), so the normal Open → Close → Destroy lifecycle
     * is unaffected. */
    TlsStream_Close(base);
    TlsStream_ReleaseSslContext(self);
    /* Overwrite the abstract base with the shared NullStream vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullStream_Get();
}

static inline struct SolidSyslogTlsStream* TlsStream_SelfFromBase(struct SolidSyslogStream* base)
{
    return (struct SolidSyslogTlsStream*) base;
}

static inline void TlsStream_ReleaseHandshakeState(struct SolidSyslogTlsStream* self)
{
    TlsStream_ReleaseSsl(self);
    TlsStream_ReleaseBioMethod(self);
}

static inline void TlsStream_ReleaseSsl(struct SolidSyslogTlsStream* self)
{
    if (self->Ssl != NULL)
    {
        SSL_free(self->Ssl);
        self->Ssl = NULL;
    }
}

static inline void TlsStream_ReleaseBioMethod(struct SolidSyslogTlsStream* self)
{
    if (self->BioMethod != NULL)
    {
        BIO_meth_free(self->BioMethod);
        self->BioMethod = NULL;
    }
}

static inline void TlsStream_ReleaseSslContext(struct SolidSyslogTlsStream* self)
{
    if (self->Ctx != NULL)
    {
        SSL_CTX_free(self->Ctx);
        self->Ctx = NULL;
    }
}

static inline bool TlsStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr)
{
    struct SolidSyslogTlsStream* self = TlsStream_SelfFromBase(base);
    return SolidSyslogStream_Open(self->Config.Transport, addr) && TlsStream_InitSslContext(self) &&
           TlsStream_InitSslSession(self) && TlsStream_AttachTransportBio(self) &&
           TlsStream_ConfigureExpectedHostname(self) && TlsStream_PerformHandshake(self);
}

static inline bool TlsStream_InitSslContext(struct SolidSyslogTlsStream* self)
{
    self->Ctx = TlsStream_CreateSslContext(&self->Config);
    return self->Ctx != NULL;
}

static inline SSL_CTX* TlsStream_CreateSslContext(const struct SolidSyslogTlsStreamConfig* config)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if ((ctx != NULL) && !TlsStream_ConfigureSslContext(ctx, config))
    {
        SSL_CTX_free(ctx);
        ctx = NULL;
    }
    return ctx;
}

static inline bool TlsStream_ConfigureSslContext(SSL_CTX* ctx, const struct SolidSyslogTlsStreamConfig* config)
{
    return TlsStream_ConfigureTrustAnchors(ctx, config->CaBundlePath) &&
           TlsStream_ConfigureClientIdentity(ctx, config) && TlsStream_ConfigureProtocolFloor(ctx) &&
           TlsStream_ConfigureCipherList(ctx, config->CipherList);
}

static inline bool TlsStream_ConfigureClientIdentity(SSL_CTX* ctx, const struct SolidSyslogTlsStreamConfig* config)
{
    bool hasCert = config->ClientCertChainPath != NULL;
    bool hasKey = config->ClientKeyPath != NULL;
    bool ok = true;
    if (hasCert != hasKey)
    {
        ok = false; /* mTLS is all-or-nothing — partial config is a setup error */
    }
    else if (hasCert)
    {
        ok = (SSL_CTX_use_certificate_chain_file(ctx, config->ClientCertChainPath) == 1) &&
             (SSL_CTX_use_PrivateKey_file(ctx, config->ClientKeyPath, SSL_FILETYPE_PEM) == 1) &&
             (SSL_CTX_check_private_key(ctx) == 1);
    }
    else
    {
        /* neither cert nor key supplied — server-auth-only TLS, ok stays true */
    }
    return ok;
}

static inline bool TlsStream_ConfigureTrustAnchors(SSL_CTX* ctx, const char* caBundlePath)
{
    bool ok = SSL_CTX_load_verify_locations(ctx, caBundlePath, NULL) == 1;
    if (ok)
    {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    }
    return ok;
}

static inline bool TlsStream_ConfigureProtocolFloor(SSL_CTX* ctx)
{
    return SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION) == 1;
}

static inline bool TlsStream_ConfigureCipherList(SSL_CTX* ctx, const char* cipherList)
{
    bool ok = true;
    if (cipherList != NULL)
    {
        ok = SSL_CTX_set_cipher_list(ctx, cipherList) == 1;
    }
    return ok;
}

static inline bool TlsStream_InitSslSession(struct SolidSyslogTlsStream* self)
{
    self->Ssl = SSL_new(self->Ctx);
    return self->Ssl != NULL;
}

static inline bool TlsStream_AttachTransportBio(struct SolidSyslogTlsStream* self)
{
    BIO* bio = TlsStream_CreateTransportBio(self);
    bool ok = bio != NULL;
    if (ok)
    {
        BIO_set_data(bio, self->Config.Transport);
        SSL_set_bio(self->Ssl, bio, bio);
    }
    return ok;
}

static inline BIO* TlsStream_CreateTransportBio(struct SolidSyslogTlsStream* self)
{
    self->BioMethod = TlsStream_CreateTransportBioMethod();
    BIO* bio = NULL;
    if (self->BioMethod != NULL)
    {
        bio = BIO_new(self->BioMethod);
        if (bio == NULL)
        {
            TlsStream_ReleaseBioMethod(self);
        }
    }
    return bio;
}

static inline BIO_METHOD* TlsStream_CreateTransportBioMethod(void)
{
    BIO_METHOD* method = BIO_meth_new(BIO_TYPE_SOURCE_SINK, "SolidSyslog transport BIO");
    if (method != NULL)
    {
        BIO_meth_set_create(method, TlsStream_TransportBioCreate);
        BIO_meth_set_read(method, TlsStream_TransportBioRead);
        BIO_meth_set_write(method, TlsStream_TransportBioWrite);
        BIO_meth_set_ctrl(method, TlsStream_TransportBioCtrl);
    }
    return method;
}

/* Called when BIO_new instantiates a BIO from our method. Marking init=1 tells
 * OpenSSL the BIO is ready for I/O; without it SSL_connect bails early. */
static inline int TlsStream_TransportBioCreate(BIO* bio)
{
    BIO_set_init(bio, 1);
    return 1;
}

/* Translate the non-blocking transport's Read contract into the OpenSSL BIO
 * contract:
 *   transport > 0 → bytes available, BIO returns the same positive count.
 *   transport = 0 → would-block. BIO must signal retry via BIO_set_retry_read
 *                  and return -1; without this, OpenSSL treats the 0 as EOF
 *                  and aborts the handshake on the first poll.
 *   transport < 0 → EOF or error. BIO returns -1 with retry flags cleared so
 *                  OpenSSL surfaces the failure rather than spinning. */
static inline int TlsStream_TransportBioRead(BIO* bio, char* buffer, int size)
{
    struct SolidSyslogStream* transport = (struct SolidSyslogStream*) BIO_get_data(bio);
    SolidSyslogSsize n = SolidSyslogStream_Read(transport, buffer, (size_t) size);
    int result = -1;

    if (n > 0)
    {
        result = (int) n;
    }
    else if (n == 0)
    {
        BIO_set_retry_read(bio);
    }
    else
    {
        BIO_clear_retry_flags(bio);
    }
    return result;
}

static inline int TlsStream_TransportBioWrite(BIO* bio, const char* buffer, int size)
{
    struct SolidSyslogStream* transport = (struct SolidSyslogStream*) BIO_get_data(bio);
    int result = -1;

    if (SolidSyslogStream_Send(transport, buffer, (size_t) size))
    {
        result = size;
    }
    else
    {
        BIO_clear_retry_flags(bio);
    }
    return result;
}

/* Minimal ctrl handler. OpenSSL calls this for a variety of control commands
 * during normal operation; returning 1 for the common lifecycle commands lets
 * SSL_connect / SSL_write / SSL_shutdown proceed. Unknown commands return 0. */
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- signature fixed by OpenSSL BIO_ctrl_fn contract
static inline long TlsStream_TransportBioCtrl(BIO* bio, int cmd, long larg, void* parg)
{
    (void) bio;
    (void) larg;
    (void) parg;
    long result = 0;
    switch (cmd)
    {
        case BIO_CTRL_FLUSH:
        case BIO_CTRL_PUSH:
        case BIO_CTRL_POP:
        case BIO_CTRL_DUP:
            result = 1;
            break;
        default:
            break;
    }
    return result;
}

static inline bool TlsStream_ConfigureExpectedHostname(struct SolidSyslogTlsStream* self)
{
    bool ok = true;
    if (self->Config.ServerName != NULL)
    {
        ok = (SSL_set_tlsext_host_name(self->Ssl, self->Config.ServerName) == 1) &&
             (SSL_set1_host(self->Ssl, self->Config.ServerName) == 1);
    }
    return ok;
}

static inline bool TlsStream_IsRetryableSslError(int err)
{
    return (err == SSL_ERROR_WANT_READ) || (err == SSL_ERROR_WANT_WRITE);
}

static inline bool TlsStream_IsHandshakeBudgetExhausted(int totalSleptMs)
{
    return totalSleptMs >= HANDSHAKE_TIMEOUT_MILLISECONDS;
}

/* Drive SSL_connect to completion under non-blocking transport. Each call may
 * return WANT_READ/WANT_WRITE while waiting for the multi-RTT handshake to
 * progress; we sleep briefly between attempts (avoiding a busy spin) until
 * either the handshake completes, hits a hard error, or the bounded budget
 * expires. */
static inline bool TlsStream_PerformHandshake(struct SolidSyslogTlsStream* self)
{
    int totalSleptMs = 0;
    bool result = false;
    bool done = false;

    while (!done)
    {
        int rc = SSL_connect(self->Ssl);
        if (rc > 0)
        {
            result = true;
            done = true;
        }
        else
        {
            int err = SSL_get_error(self->Ssl, rc);
            if (!TlsStream_IsRetryableSslError(err) || TlsStream_IsHandshakeBudgetExhausted(totalSleptMs))
            {
                done = true;
            }
            else
            {
                self->Config.Sleep(HANDSHAKE_POLL_INTERVAL_MILLISECONDS);
                totalSleptMs += HANDSHAKE_POLL_INTERVAL_MILLISECONDS;
            }
        }
    }
    return result;
}

static inline bool TlsStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size)
{
    struct SolidSyslogTlsStream* self = TlsStream_SelfFromBase(base);
    int rc = SSL_write(self->Ssl, buffer, (int) size);
    bool ok = (rc > 0) && ((size_t) rc == size);

    if (!ok)
    {
        TlsStream_Close(base);
    }
    return ok;
}

/* SSL_read has two distinct modes worth keeping straight:
 *   1. Steady-state application read: bytes available → return them; nothing
 *      to read right now → SSL_ERROR_WANT_READ → return 0 mirrors the transport
 *      Read contract.
 *   2. Renegotiation or alerts mid-stream: SSL_read may need to write (server
 *      requested re-key), surfacing as SSL_ERROR_WANT_WRITE. Under fail-fast
 *      semantics this is a transport failure — close internally; the caller
 *      reopens, store-and-forward replays. Same rule for any other SSL error.
 * Anything below the WANT_READ branch therefore takes the Close path. */
static inline SolidSyslogSsize TlsStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size)
{
    struct SolidSyslogTlsStream* self = TlsStream_SelfFromBase(base);
    int rc = SSL_read(self->Ssl, buffer, (int) size);
    SolidSyslogSsize result = -1;

    if (rc > 0)
    {
        result = (SolidSyslogSsize) rc;
    }
    else if (SSL_get_error(self->Ssl, rc) == SSL_ERROR_WANT_READ)
    {
        result = 0;
    }
    else
    {
        TlsStream_Close(base);
    }
    return result;
}

/* Idempotent: Send/Read may close internally on failure, after which the
 * StreamSender's reconnect path or the caller's Destroy may call Close
 * again. Skipping when ssl is already NULL keeps that safe. */
static inline void TlsStream_Close(struct SolidSyslogStream* base)
{
    struct SolidSyslogTlsStream* self = TlsStream_SelfFromBase(base);
    if (self->Ssl != NULL)
    {
        SSL_shutdown(self->Ssl);
        TlsStream_ReleaseHandshakeState(self);
    }
    SolidSyslogStream_Close(self->Config.Transport);
}
