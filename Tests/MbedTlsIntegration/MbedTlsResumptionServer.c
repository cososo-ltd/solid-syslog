#include "MbedTlsResumptionServer.h"

#include <mbedtls/cipher.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_ticket.h>
#include <mbedtls/x509_crt.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "MbedTlsTestCert.h"
#include "mbedtls/pk.h"

enum
{
    CONNECTION_COUNT = 2,
    TICKET_LIFETIME_SECONDS = 86400,
    APP_RECORD_BUFFER_BYTES = 512
};

/* Shared by the wrapped ticket write / parse callbacks. `Ctx` is the real
 * ticket context the wrappers forward to; swapping it before the second
 * connection is how the rotate-key (non-resuming-peer) case is staged.
 * `Accepted` is set true when a presented ticket parses successfully — the
 * stand-in for mbedTLS's missing client-side SSL_session_reused(). */
struct TicketCallbackContext
{
    mbedtls_ssl_ticket_context* Ctx;
    bool Accepted;
};

struct MbedTlsResumptionServer
{
    int Fds[CONNECTION_COUNT];
    bool RotateKey;
    mbedtls_ctr_drbg_context* Rng;
    const struct MbedTlsTestCert* ServerCert;
    mbedtls_ssl_config Conf;
    mbedtls_ssl_ticket_context TicketCtxPrimary;
    mbedtls_ssl_ticket_context TicketCtxRotated;
    struct TicketCallbackContext TicketCb;
    pthread_t Thread;
    bool Joined;
    bool HandshakeSucceeded[CONNECTION_COUNT];
    bool MessageDelivered[CONNECTION_COUNT];
    bool ResumeAccepted[CONNECTION_COUNT];
};

static void* RunServer(void* arg);
static int WrappedTicketWrite(
    void* p_ticket,
    const mbedtls_ssl_session* session,
    unsigned char* start,
    const unsigned char* end,
    size_t* tlen,
    uint32_t* lifetime
);
static int WrappedTicketParse(void* p_ticket, mbedtls_ssl_session* session, unsigned char* buf, size_t len);
static int ServerBioSend(void* ctx, const unsigned char* buf, size_t len);
static int ServerBioRecv(void* ctx, unsigned char* buf, size_t len);

struct MbedTlsResumptionServer* MbedTlsResumptionServer_Create(const struct MbedTlsResumptionServerConfig* config)
{
    struct MbedTlsResumptionServer* self =
        (struct MbedTlsResumptionServer*) calloc(1U, sizeof(struct MbedTlsResumptionServer));
    self->Fds[0] = config->ServerFds[0];
    self->Fds[1] = config->ServerFds[1];
    self->RotateKey = config->RotateTicketKeyBetweenConnections;
    self->Rng = config->Rng;
    self->ServerCert = config->ServerCert;

    mbedtls_ssl_config_init(&self->Conf);
    mbedtls_ssl_config_defaults(
        &self->Conf,
        MBEDTLS_SSL_IS_SERVER,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT
    );
    /* Pin TLS 1.2 — same rationale as MbedTlsTestServer: keeps the rejection /
     * teardown cadence synchronous over a socketpair, and the TLS 1.2 ticket
     * arrives in-handshake so the client can capture it immediately. */
    mbedtls_ssl_conf_max_tls_version(&self->Conf, MBEDTLS_SSL_VERSION_TLS1_2);
    mbedtls_ssl_conf_rng(&self->Conf, mbedtls_ctr_drbg_random, self->Rng);
    /* Server-auth only — these tests exercise resumption, not client identity. */
    mbedtls_ssl_conf_authmode(&self->Conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_own_cert(
        &self->Conf,
        (mbedtls_x509_crt*) &self->ServerCert->Cert,
        (mbedtls_pk_context*) &self->ServerCert->Key
    );

    mbedtls_ssl_ticket_init(&self->TicketCtxPrimary);
    mbedtls_ssl_ticket_setup(
        &self->TicketCtxPrimary,
        mbedtls_ctr_drbg_random,
        self->Rng,
        MBEDTLS_CIPHER_AES_256_GCM,
        TICKET_LIFETIME_SECONDS
    );
    mbedtls_ssl_ticket_init(&self->TicketCtxRotated); /* setup lazily in RunServer if rotating */
    self->TicketCb.Ctx = &self->TicketCtxPrimary;
    self->TicketCb.Accepted = false;
    mbedtls_ssl_conf_session_tickets_cb(&self->Conf, WrappedTicketWrite, WrappedTicketParse, &self->TicketCb);

    pthread_create(&self->Thread, NULL, RunServer, self);
    return self;
}

void MbedTlsResumptionServer_Destroy(struct MbedTlsResumptionServer* self)
{
    if (self != NULL)
    {
        if (!self->Joined)
        {
            /* Unblock a worker that may be parked in recv on a wedged
             * negative-path test, then join. */
            for (int i = 0; i < CONNECTION_COUNT; i++)
            {
                if (self->Fds[i] >= 0)
                {
                    shutdown(self->Fds[i], SHUT_RDWR);
                }
            }
            pthread_join(self->Thread, NULL);
            self->Joined = true;
        }
        mbedtls_ssl_config_free(&self->Conf);
        mbedtls_ssl_ticket_free(&self->TicketCtxPrimary);
        mbedtls_ssl_ticket_free(&self->TicketCtxRotated);
        for (int i = 0; i < CONNECTION_COUNT; i++)
        {
            if (self->Fds[i] >= 0)
            {
                close(self->Fds[i]);
            }
        }
        free(self);
    }
}

void MbedTlsResumptionServer_Join(struct MbedTlsResumptionServer* self)
{
    if (!self->Joined)
    {
        pthread_join(self->Thread, NULL);
        self->Joined = true;
    }
}

bool MbedTlsResumptionServer_BothHandshakesSucceeded(struct MbedTlsResumptionServer* self)
{
    return self->HandshakeSucceeded[0] && self->HandshakeSucceeded[1];
}

bool MbedTlsResumptionServer_BothMessagesDelivered(struct MbedTlsResumptionServer* self)
{
    return self->MessageDelivered[0] && self->MessageDelivered[1];
}

bool MbedTlsResumptionServer_SecondHandshakeResumed(struct MbedTlsResumptionServer* self)
{
    return self->ResumeAccepted[1];
}

/* Drives two consecutive server-side handshakes — one per fd — reading a
 * single application record after each so the test can assert delivery on
 * both the full and the resumed connection. */
static void* RunServer(void* arg)
{
    struct MbedTlsResumptionServer* self = (struct MbedTlsResumptionServer*) arg;

    for (int i = 0; i < CONNECTION_COUNT; i++)
    {
        if ((i == 1) && self->RotateKey)
        {
            /* Fresh key the client's saved ticket cannot decrypt -> the second
             * handshake falls back to full. */
            mbedtls_ssl_ticket_setup(
                &self->TicketCtxRotated,
                mbedtls_ctr_drbg_random,
                self->Rng,
                MBEDTLS_CIPHER_AES_256_GCM,
                TICKET_LIFETIME_SECONDS
            );
            self->TicketCb.Ctx = &self->TicketCtxRotated;
        }
        self->TicketCb.Accepted = false;

        mbedtls_ssl_context ssl;
        mbedtls_ssl_init(&ssl);
        mbedtls_ssl_setup(&ssl, &self->Conf);
        mbedtls_ssl_set_bio(&ssl, &self->Fds[i], ServerBioSend, ServerBioRecv, NULL);

        int handshakeRc = 0;
        do
        {
            handshakeRc = mbedtls_ssl_handshake(&ssl);
        } while ((handshakeRc == MBEDTLS_ERR_SSL_WANT_READ) || (handshakeRc == MBEDTLS_ERR_SSL_WANT_WRITE));

        self->HandshakeSucceeded[i] = (handshakeRc == 0);
        self->ResumeAccepted[i] = self->TicketCb.Accepted;

        if (handshakeRc == 0)
        {
            unsigned char buffer[APP_RECORD_BUFFER_BYTES];
            int readRc = 0;
            do
            {
                readRc = mbedtls_ssl_read(&ssl, buffer, sizeof(buffer));
            } while ((readRc == MBEDTLS_ERR_SSL_WANT_READ) || (readRc == MBEDTLS_ERR_SSL_WANT_WRITE));
            self->MessageDelivered[i] = (readRc > 0);
        }

        mbedtls_ssl_free(&ssl);
    }
    return NULL;
}

static int WrappedTicketWrite(
    void* p_ticket,
    const mbedtls_ssl_session* session,
    unsigned char* start,
    const unsigned char* end,
    size_t* tlen,
    uint32_t* lifetime
)
{
    struct TicketCallbackContext* cb = (struct TicketCallbackContext*) p_ticket;
    return mbedtls_ssl_ticket_write(cb->Ctx, session, start, end, tlen, lifetime);
}

static int WrappedTicketParse(void* p_ticket, mbedtls_ssl_session* session, unsigned char* buf, size_t len)
{
    struct TicketCallbackContext* cb = (struct TicketCallbackContext*) p_ticket;
    int rc = mbedtls_ssl_ticket_parse(cb->Ctx, session, buf, len);
    if (rc == 0)
    {
        cb->Accepted = true; /* a valid ticket was presented -> this handshake resumed */
    }
    return rc;
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
