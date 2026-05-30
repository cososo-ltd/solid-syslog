#ifndef MBEDTLSRESUMPTIONSERVER_H
#define MBEDTLSRESUMPTIONSERVER_H

#include <mbedtls/ctr_drbg.h>
#include <stdbool.h>

#include "ExternC.h"

struct MbedTlsTestCert;

EXTERN_C_BEGIN

    struct MbedTlsResumptionServer;

    struct MbedTlsResumptionServerConfig
    {
        int ServerFds[2]; /* the two socketpair server ends; ownership transfers to the server */
        const struct MbedTlsTestCert* ServerCert; /* server cert + matching key */
        mbedtls_ctr_drbg_context* Rng; /* shared with the test fixture */
        /* false: one ticket key shared across both connections -> the second
         *        handshake resumes.
         * true:  the ticket key is rotated before the second connection so the
         *        client's saved ticket no longer decrypts -> the server falls
         *        back to a full handshake (the non-resuming-peer case). */
        bool RotateTicketKeyBetweenConnections;
    };

    /* Spawns a worker that issues session tickets and drives two consecutive
     * server-side handshakes (one per ServerFds entry), then reads one
     * application record on each. The server owns both fds from Create. */
    struct MbedTlsResumptionServer* MbedTlsResumptionServer_Create(const struct MbedTlsResumptionServerConfig* config);

    /* Joins the worker and tears down all mbedTLS state. */
    void MbedTlsResumptionServer_Destroy(struct MbedTlsResumptionServer * self);

    /* Joins the worker thread. Callable once; the getters below are valid
     * afterwards. */
    void MbedTlsResumptionServer_Join(struct MbedTlsResumptionServer * self);

    /* True when both handshakes completed successfully. */
    bool MbedTlsResumptionServer_BothHandshakesSucceeded(struct MbedTlsResumptionServer * self);

    /* True when an application record was received on both connections —
     * proves delivery survives whichever handshake path was taken. */
    bool MbedTlsResumptionServer_BothMessagesDelivered(struct MbedTlsResumptionServer * self);

    /* The observable: true when the SECOND handshake presented a ticket the
     * server accepted (an abbreviated / resumed handshake). mbedTLS exposes no
     * client-side SSL_session_reused() equivalent, so the server records this
     * by wrapping its ticket-parse callback. */
    bool MbedTlsResumptionServer_SecondHandshakeResumed(struct MbedTlsResumptionServer * self);

EXTERN_C_END

#endif /* MBEDTLSRESUMPTIONSERVER_H */
