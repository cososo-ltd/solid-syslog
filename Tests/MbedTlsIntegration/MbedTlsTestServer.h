#ifndef MBEDTLSTESTSERVER_H
#define MBEDTLSTESTSERVER_H

#include <mbedtls/ctr_drbg.h>
#include <stdbool.h>
#include <stddef.h>

#include "ExternC.h"

struct MbedTlsTestCert;

EXTERN_C_BEGIN

    struct MbedTlsTestServer;

    struct MbedTlsTestServerConfig
    {
        int ServerFd; /* one end of a socketpair; ownership transferred to the server */
        const struct MbedTlsTestCert* ServerCert; /* server's cert + matching key */
        mbedtls_ctr_drbg_context* Rng; /* shared with the test fixture */
    };

    /* Spawns a worker thread that drives the server-side handshake and then
     * reads bytes until the peer closes. The Server owns ServerFd from this
     * point on. */
    struct MbedTlsTestServer* MbedTlsTestServer_Create(const struct MbedTlsTestServerConfig* config);

    /* Joins the worker thread and tears down all mbedTLS state. */
    void MbedTlsTestServer_Destroy(struct MbedTlsTestServer * self);

    /* Wait for the worker thread to exit and return whether the handshake
     * completed successfully. Callable once per server. */
    bool MbedTlsTestServer_JoinAndHandshakeSucceeded(struct MbedTlsTestServer * self);

    /* After Join: the bytes the server read via mbedtls_ssl_read after a
     * successful handshake (zero-length if the handshake failed or the peer
     * closed without sending). */
    const unsigned char* MbedTlsTestServer_ReceivedBytes(struct MbedTlsTestServer * self);
    size_t MbedTlsTestServer_ReceivedLength(struct MbedTlsTestServer * self);

EXTERN_C_END

#endif /* MBEDTLSTESTSERVER_H */
