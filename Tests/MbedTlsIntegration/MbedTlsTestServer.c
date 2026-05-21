#include "MbedTlsTestServer.h"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "MbedTlsTestCert.h"

enum
{
    RECEIVE_BUFFER_BYTES = 1024
};

struct MbedTlsTestServer
{
    int Fd;
    mbedtls_ssl_config SslConfig;
    mbedtls_ssl_context SslContext;
    pthread_t Thread;
    bool ThreadJoined;
    bool HandshakeSucceeded;
    unsigned char Received[RECEIVE_BUFFER_BYTES];
    size_t ReceivedLength;
};

static void* RunServer(void* arg);
static int ServerBioSend(void* ctx, const unsigned char* buf, size_t len);
static int ServerBioRecv(void* ctx, unsigned char* buf, size_t len);

struct MbedTlsTestServer* MbedTlsTestServer_Create(const struct MbedTlsTestServerConfig* config)
{
    struct MbedTlsTestServer* self = (struct MbedTlsTestServer*) calloc(1U, sizeof(struct MbedTlsTestServer));
    self->Fd = config->ServerFd;

    mbedtls_ssl_config_init(&self->SslConfig);
    mbedtls_ssl_config_defaults(
        &self->SslConfig,
        MBEDTLS_SSL_IS_SERVER,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT
    );
    mbedtls_ssl_conf_rng(&self->SslConfig, mbedtls_ctr_drbg_random, config->Rng);
    /* Server-auth-only: don't require a client cert in slice 3 (mTLS lands
     * in slice 5). */
    mbedtls_ssl_conf_authmode(&self->SslConfig, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_own_cert(
        &self->SslConfig,
        (mbedtls_x509_crt*) &config->ServerCert->Cert,
        (mbedtls_pk_context*) &config->ServerCert->Key
    );

    mbedtls_ssl_init(&self->SslContext);
    mbedtls_ssl_setup(&self->SslContext, &self->SslConfig);
    mbedtls_ssl_set_bio(&self->SslContext, &self->Fd, ServerBioSend, ServerBioRecv, NULL);

    pthread_create(&self->Thread, NULL, RunServer, self);
    return self;
}

void MbedTlsTestServer_Destroy(struct MbedTlsTestServer* self)
{
    if (self == NULL)
    {
        return;
    }
    if (!self->ThreadJoined)
    {
        /* Worker might still be blocked in recv. Shutting the fd unblocks
         * it; close gets called below after the join. */
        if (self->Fd >= 0)
        {
            shutdown(self->Fd, SHUT_RDWR);
        }
        pthread_join(self->Thread, NULL);
        self->ThreadJoined = true;
    }
    mbedtls_ssl_free(&self->SslContext);
    mbedtls_ssl_config_free(&self->SslConfig);
    if (self->Fd >= 0)
    {
        close(self->Fd);
    }
    free(self);
}

bool MbedTlsTestServer_JoinAndHandshakeSucceeded(struct MbedTlsTestServer* self)
{
    /* RunServer exits naturally once the client closes its end (recv returns
     * 0 → handshake or read sees EOF). Tests should close the client side
     * before calling Join so this returns promptly. */
    if (!self->ThreadJoined)
    {
        pthread_join(self->Thread, NULL);
        self->ThreadJoined = true;
    }
    return self->HandshakeSucceeded;
}

const unsigned char* MbedTlsTestServer_ReceivedBytes(struct MbedTlsTestServer* self)
{
    return self->Received;
}

size_t MbedTlsTestServer_ReceivedLength(struct MbedTlsTestServer* self)
{
    return self->ReceivedLength;
}

static void* RunServer(void* arg)
{
    struct MbedTlsTestServer* self = (struct MbedTlsTestServer*) arg;

    int handshakeRc = 0;
    do
    {
        handshakeRc = mbedtls_ssl_handshake(&self->SslContext);
    } while ((handshakeRc == MBEDTLS_ERR_SSL_WANT_READ) || (handshakeRc == MBEDTLS_ERR_SSL_WANT_WRITE));

    self->HandshakeSucceeded = (handshakeRc == 0);

    /* Slice 3 only pins handshake outcome; the thread exits as soon as the
     * handshake settles. Reading TLS bytes from the server side (and any
     * blocking that implies) is left for a future slice when a test actually
     * needs it. */
    return NULL;
}

static int ServerBioSend(void* ctx, const unsigned char* buf, size_t len)
{
    int fd = *(int*) ctx;
    ssize_t n = send(fd, buf, len, 0);
    return (n >= 0) ? (int) n : -1;
}

static int ServerBioRecv(void* ctx, unsigned char* buf, size_t len)
{
    int fd = *(int*) ctx;
    ssize_t n = recv(fd, buf, len, 0);
    return (n >= 0) ? (int) n : -1;
}
