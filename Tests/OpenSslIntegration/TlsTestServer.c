#include "TlsTestServer.h"

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <stdbool.h>
#include <stdlib.h>
#include <openssl/prov_ssl.h>

#include "TlsTestCert.h"

struct TlsTestServer
{
    SSL_CTX* ctx;
    SSL*     ssl;
    BIO*     serverBio;
    BIO*     clientBio;
    bool     handshakeComplete;
};

struct TlsTestServer* TlsTestServer_Create(const struct TlsTestServerConfig* config)
{
    struct TlsTestServer* self = (struct TlsTestServer*) calloc(1, sizeof(struct TlsTestServer));

    self->ctx = SSL_CTX_new(TLS_server_method());
    SSL_CTX_use_certificate(self->ctx, config->serverCert->cert);
    SSL_CTX_use_PrivateKey(self->ctx, config->serverCert->key);
    if (config->cipherList != NULL)
    {
        SSL_CTX_set_cipher_list(self->ctx, config->cipherList);
    }
    if (config->clientCaCert != NULL)
    {
        X509_STORE* store = SSL_CTX_get_cert_store(self->ctx);
        X509_STORE_add_cert(store, config->clientCaCert->cert);
        SSL_CTX_set_verify(self->ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
        /* Pin TLS 1.2 for mTLS tests — in TLS 1.3 the client's SSL_connect
         * can return before the server's verify completes, so a cert rejection
         * on the server shows up on the client only on the next read.
         * TLS 1.2 keeps rejection synchronous within the handshake. */
        SSL_CTX_set_max_proto_version(self->ctx, TLS1_2_VERSION);
    }

    self->ssl = SSL_new(self->ctx);
    BIO_new_bio_pair(&self->serverBio, 0, &self->clientBio, 0);
    SSL_set_bio(self->ssl, self->serverBio, self->serverBio);
    self->serverBio = NULL; /* ownership transferred to SSL via SSL_set_bio */
    SSL_set_accept_state(self->ssl);

    return self;
}

void TlsTestServer_Destroy(struct TlsTestServer* self)
{
    if (self == NULL)
    {
        return;
    }
    if (self->ssl != NULL)
    {
        SSL_free(self->ssl); /* also frees serverBio */
    }
    if (self->clientBio != NULL)
    {
        BIO_free(self->clientBio);
    }
    if (self->ctx != NULL)
    {
        SSL_CTX_free(self->ctx);
    }
    free(self);
}

BIO* TlsTestServer_ClientSideBio(struct TlsTestServer* self)
{
    return self->clientBio;
}

void TlsTestServer_Pump(void* context)
{
    struct TlsTestServer* self = (struct TlsTestServer*) context;
    if (!self->handshakeComplete)
    {
        int ret = SSL_accept(self->ssl);
        if (ret == 1)
        {
            self->handshakeComplete = true;
        }
    }
}
