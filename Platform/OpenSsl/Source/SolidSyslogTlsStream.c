#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/prov_ssl.h>
#include <openssl/types.h>
#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogMacros.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTlsStream.h"
#include "SolidSyslogStream.h"

struct SolidSyslogAddress;

struct SolidSyslogTlsStream
{
    struct SolidSyslogStream          base;
    struct SolidSyslogTlsStreamConfig config;
    SSL_CTX*                          ctx;
    SSL*                              ssl;
    BIO_METHOD*                       bioMethod;
};

SOLIDSYSLOG_STATIC_ASSERT(sizeof(struct SolidSyslogTlsStream) <= sizeof(SolidSyslogTlsStreamStorage),
                          "SOLIDSYSLOG_TLS_STREAM_SIZE is too small for struct SolidSyslogTlsStream");

static inline bool             TlsStream_AttachTransportBio(struct SolidSyslogTlsStream* stream);
static inline void             TlsStream_Close(struct SolidSyslogStream* self);
static inline bool             TlsStream_ConfigureCipherList(SSL_CTX* ctx, const char* cipherList);
static inline bool             TlsStream_ConfigureClientIdentity(SSL_CTX* ctx, const struct SolidSyslogTlsStreamConfig* config);
static inline bool             TlsStream_ConfigureExpectedHostname(struct SolidSyslogTlsStream* stream);
static inline bool             TlsStream_ConfigureProtocolFloor(SSL_CTX* ctx);
static inline bool             TlsStream_ConfigureSslContext(SSL_CTX* ctx, const struct SolidSyslogTlsStreamConfig* config);
static inline bool             TlsStream_ConfigureTrustAnchors(SSL_CTX* ctx, const char* caBundlePath);
static inline SSL_CTX*         TlsStream_CreateSslContext(const struct SolidSyslogTlsStreamConfig* config);
static inline BIO*             TlsStream_CreateTransportBio(struct SolidSyslogTlsStream* stream);
static inline BIO_METHOD*      TlsStream_CreateTransportBioMethod(void);
static inline bool             TlsStream_InitSslContext(struct SolidSyslogTlsStream* stream);
static inline bool             TlsStream_InitSslSession(struct SolidSyslogTlsStream* stream);
static inline bool             TlsStream_Open(struct SolidSyslogStream* self, const struct SolidSyslogAddress* addr);
static inline bool             TlsStream_PerformHandshake(struct SolidSyslogTlsStream* stream);
static inline SolidSyslogSsize TlsStream_Read(struct SolidSyslogStream* self, void* buffer, size_t size);
static inline void             TlsStream_ReleaseBioMethod(struct SolidSyslogTlsStream* stream);
static inline void             TlsStream_ReleaseHandshakeState(struct SolidSyslogTlsStream* stream);
static inline void             TlsStream_ReleaseSsl(struct SolidSyslogTlsStream* stream);
static inline void             TlsStream_ReleaseSslContext(struct SolidSyslogTlsStream* stream);
static inline bool             TlsStream_Send(struct SolidSyslogStream* self, const void* buffer, size_t size);
static inline int              TlsStream_TransportBioCreate(BIO* bio);
static inline long             TlsStream_TransportBioCtrl(BIO* bio, int cmd, long larg, void* parg);
static inline int              TlsStream_TransportBioRead(BIO* bio, char* buffer, int size);
static inline int              TlsStream_TransportBioWrite(BIO* bio, const char* buffer, int size);

static const struct SolidSyslogTlsStream DEFAULT_INSTANCE = {
    {TlsStream_Open, TlsStream_Send, TlsStream_Read, TlsStream_Close}, {NULL, NULL, NULL, NULL, NULL, NULL}, NULL, NULL, NULL,
};

static const struct SolidSyslogTlsStream DESTROYED_INSTANCE = {
    {NULL, NULL, NULL, NULL}, {NULL, NULL, NULL, NULL, NULL, NULL}, NULL, NULL, NULL,
};

struct SolidSyslogStream* SolidSyslogTlsStream_Create(SolidSyslogTlsStreamStorage* storage, const struct SolidSyslogTlsStreamConfig* config)
{
    struct SolidSyslogTlsStream* stream = (struct SolidSyslogTlsStream*) storage;
    *stream                             = DEFAULT_INSTANCE;
    stream->config                      = *config;
    return &stream->base;
}

void SolidSyslogTlsStream_Destroy(struct SolidSyslogStream* stream)
{
    struct SolidSyslogTlsStream* self = (struct SolidSyslogTlsStream*) stream;
    TlsStream_ReleaseHandshakeState(self);
    TlsStream_ReleaseSslContext(self);
    *self = DESTROYED_INSTANCE;
}

static inline void TlsStream_ReleaseHandshakeState(struct SolidSyslogTlsStream* stream)
{
    TlsStream_ReleaseSsl(stream);
    TlsStream_ReleaseBioMethod(stream);
}

static inline void TlsStream_ReleaseSsl(struct SolidSyslogTlsStream* stream)
{
    if (stream->ssl != NULL)
    {
        SSL_free(stream->ssl);
        stream->ssl = NULL;
    }
}

static inline void TlsStream_ReleaseBioMethod(struct SolidSyslogTlsStream* stream)
{
    if (stream->bioMethod != NULL)
    {
        BIO_meth_free(stream->bioMethod);
        stream->bioMethod = NULL;
    }
}

static inline void TlsStream_ReleaseSslContext(struct SolidSyslogTlsStream* stream)
{
    if (stream->ctx != NULL)
    {
        SSL_CTX_free(stream->ctx);
        stream->ctx = NULL;
    }
}

static inline bool TlsStream_Open(struct SolidSyslogStream* self, const struct SolidSyslogAddress* addr)
{
    struct SolidSyslogTlsStream* stream = (struct SolidSyslogTlsStream*) self;
    return SolidSyslogStream_Open(stream->config.transport, addr) && TlsStream_InitSslContext(stream) && TlsStream_InitSslSession(stream) &&
           TlsStream_AttachTransportBio(stream) && TlsStream_ConfigureExpectedHostname(stream) && TlsStream_PerformHandshake(stream);
}

static inline bool TlsStream_InitSslContext(struct SolidSyslogTlsStream* stream)
{
    stream->ctx = TlsStream_CreateSslContext(&stream->config);
    return stream->ctx != NULL;
}

static inline SSL_CTX* TlsStream_CreateSslContext(const struct SolidSyslogTlsStreamConfig* config)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if (ctx != NULL && !TlsStream_ConfigureSslContext(ctx, config))
    {
        SSL_CTX_free(ctx);
        ctx = NULL;
    }
    return ctx;
}

static inline bool TlsStream_ConfigureSslContext(SSL_CTX* ctx, const struct SolidSyslogTlsStreamConfig* config)
{
    return TlsStream_ConfigureTrustAnchors(ctx, config->caBundlePath) && TlsStream_ConfigureClientIdentity(ctx, config) &&
           TlsStream_ConfigureProtocolFloor(ctx) && TlsStream_ConfigureCipherList(ctx, config->cipherList);
}

static inline bool TlsStream_ConfigureClientIdentity(SSL_CTX* ctx, const struct SolidSyslogTlsStreamConfig* config)
{
    bool hasCert = config->clientCertChainPath != NULL;
    bool hasKey  = config->clientKeyPath != NULL;
    bool ok      = true;
    if (hasCert != hasKey)
    {
        ok = false; /* mTLS is all-or-nothing — partial config is a setup error */
    }
    else if (hasCert)
    {
        ok = (SSL_CTX_use_certificate_chain_file(ctx, config->clientCertChainPath) == 1) &&
             (SSL_CTX_use_PrivateKey_file(ctx, config->clientKeyPath, SSL_FILETYPE_PEM) == 1) && (SSL_CTX_check_private_key(ctx) == 1);
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

static inline bool TlsStream_InitSslSession(struct SolidSyslogTlsStream* stream)
{
    stream->ssl = SSL_new(stream->ctx);
    return stream->ssl != NULL;
}

static inline bool TlsStream_AttachTransportBio(struct SolidSyslogTlsStream* stream)
{
    BIO* bio = TlsStream_CreateTransportBio(stream);
    bool ok  = bio != NULL;
    if (ok)
    {
        BIO_set_data(bio, stream->config.transport);
        SSL_set_bio(stream->ssl, bio, bio);
    }
    return ok;
}

static inline BIO* TlsStream_CreateTransportBio(struct SolidSyslogTlsStream* stream)
{
    stream->bioMethod = TlsStream_CreateTransportBioMethod();
    BIO* bio          = NULL;
    if (stream->bioMethod != NULL)
    {
        bio = BIO_new(stream->bioMethod);
        if (bio == NULL)
        {
            TlsStream_ReleaseBioMethod(stream);
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

static inline int TlsStream_TransportBioRead(BIO* bio, char* buffer, int size)
{
    struct SolidSyslogStream* transport = (struct SolidSyslogStream*) BIO_get_data(bio);
    return (int) SolidSyslogStream_Read(transport, buffer, (size_t) size);
}

static inline int TlsStream_TransportBioWrite(BIO* bio, const char* buffer, int size)
{
    struct SolidSyslogStream* transport = (struct SolidSyslogStream*) BIO_get_data(bio);
    return SolidSyslogStream_Send(transport, buffer, (size_t) size) ? size : -1;
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

static inline bool TlsStream_ConfigureExpectedHostname(struct SolidSyslogTlsStream* stream)
{
    bool ok = true;
    if (stream->config.serverName != NULL)
    {
        ok = (SSL_set_tlsext_host_name(stream->ssl, stream->config.serverName) == 1) && (SSL_set1_host(stream->ssl, stream->config.serverName) == 1);
    }
    return ok;
}

static inline bool TlsStream_PerformHandshake(struct SolidSyslogTlsStream* stream)
{
    return SSL_connect(stream->ssl) > 0;
}

static inline bool TlsStream_Send(struct SolidSyslogStream* self, const void* buffer, size_t size)
{
    struct SolidSyslogTlsStream* stream = (struct SolidSyslogTlsStream*) self;
    return SSL_write(stream->ssl, buffer, (int) size) > 0;
}

static inline SolidSyslogSsize TlsStream_Read(struct SolidSyslogStream* self, void* buffer, size_t size)
{
    struct SolidSyslogTlsStream* stream = (struct SolidSyslogTlsStream*) self;
    return (SolidSyslogSsize) SSL_read(stream->ssl, buffer, (int) size);
}

static inline void TlsStream_Close(struct SolidSyslogStream* self)
{
    struct SolidSyslogTlsStream* stream = (struct SolidSyslogTlsStream*) self;
    SSL_shutdown(stream->ssl);
    TlsStream_ReleaseHandshakeState(stream);
    SolidSyslogStream_Close(stream->config.transport);
}
