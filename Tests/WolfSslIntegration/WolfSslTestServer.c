/* An in-process TLS server for the wolfSSL stream integration tests: one
 * blocking socketpair end, a TLS 1.2 handshake on a worker thread, then one
 * read. TLS 1.2 rather than 1.3 for the same reason the other packs' test
 * servers pin it: its request/response cadence keeps a refusal synchronous, so
 * the worker sees the client close and exits rather than blocking in recv
 * after a one-flight server hello. */
#include "WolfSslTestServer.h"

#ifndef WOLFSSL_USER_SETTINGS
#include <wolfssl/options.h>
#endif
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wolfssl/ssl.h>

enum
{
    WOLFSSLTESTSERVER_MAX_RECEIVED = 256
};

struct WolfSslTestServer
{
    int Fd;
    WOLFSSL_CTX* Context;
    WOLFSSL* Session;
    pthread_t Thread;
    bool ThreadJoined;
    bool HandshakeSucceeded;
    char Received[WOLFSSLTESTSERVER_MAX_RECEIVED];
};

static void* RunServer(void* arg);
static void Join(struct WolfSslTestServer* self);

struct WolfSslTestServer* WolfSslTestServer_Create(const struct WolfSslTestServerConfig* config)
{
    struct WolfSslTestServer* self = (struct WolfSslTestServer*) calloc(1U, sizeof(struct WolfSslTestServer));
    self->Fd = config->ServerFd;
    self->Context = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
    (void) wolfSSL_CTX_use_certificate_chain_file(self->Context, config->CertificatePath);
    (void) wolfSSL_CTX_use_PrivateKey_file(self->Context, config->KeyPath, WOLFSSL_FILETYPE_PEM);
    wolfSSL_CTX_set_verify(self->Context, WOLFSSL_VERIFY_NONE, NULL);
    self->Session = wolfSSL_new(self->Context);
    (void) wolfSSL_set_fd(self->Session, self->Fd);
    pthread_create(&self->Thread, NULL, RunServer, self);
    return self;
}

void WolfSslTestServer_Destroy(struct WolfSslTestServer* self)
{
    if (self != NULL)
    {
        if (!self->ThreadJoined)
        {
            /* The worker might still be blocked in recv; shutting the fd
             * unblocks it, and close follows the join. */
            shutdown(self->Fd, SHUT_RDWR);
            Join(self);
        }
        wolfSSL_free(self->Session);
        wolfSSL_CTX_free(self->Context);
        close(self->Fd);
        free(self);
    }
}

bool WolfSslTestServer_JoinAndHandshakeSucceeded(struct WolfSslTestServer* self)
{
    Join(self);
    return self->HandshakeSucceeded;
}

const char* WolfSslTestServer_Received(struct WolfSslTestServer* self)
{
    Join(self);
    return self->Received;
}

/* The worker exits once the client closes its end: the tests close the client
 * side before joining, so this returns promptly. */
static void Join(struct WolfSslTestServer* self)
{
    if (!self->ThreadJoined)
    {
        pthread_join(self->Thread, NULL);
        self->ThreadJoined = true;
    }
}

static void* RunServer(void* arg)
{
    struct WolfSslTestServer* self = (struct WolfSslTestServer*) arg;
    self->HandshakeSucceeded = wolfSSL_accept(self->Session) == WOLFSSL_SUCCESS;
    if (self->HandshakeSucceeded)
    {
        int n = wolfSSL_read(self->Session, self->Received, (int) sizeof(self->Received) - 1);
        if (n > 0)
        {
            self->Received[n] = '\0';
        }
    }
    return NULL;
}
