#include "CppUTest/TestHarness.h"

extern "C"
{
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/version.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "MbedTlsResumptionServer.h"
#include "MbedTlsTestCert.h"
#include "MbedTlsTestServer.h"
#include "ReconnectingSocketStream.h"
#include "SocketStream.h"
#include "SolidSyslogMbedTlsStream.h"
#include "AddressFake.h"
#include "SolidSyslogStream.h"
}

namespace
{
constexpr const char* TEST_SERVER_HOSTNAME = "syslog.example.com";
constexpr const char* TEST_CA_SUBJECT = "CN=Test Root CA";
constexpr const char* TEST_SERVER_SUBJECT = "CN=syslog.example.com";

void NoOpSleep(int milliseconds)
{
    (void) milliseconds;
}
} // namespace

// clang-format off
TEST_GROUP(SolidSyslogMbedTlsStreamIntegration)
{
    mbedtls_entropy_context  entropy            = {};
    mbedtls_ctr_drbg_context rng                = {};
    int                      fds[2]             = {-1, -1};
    struct MbedTlsTestCert   trustedCa          = {};
    struct MbedTlsTestCert   serverCert         = {};
    struct MbedTlsTestServer* server            = nullptr;
    struct SolidSyslogStream* clientTransport   = nullptr;
    struct SolidSyslogStream* tlsStream         = nullptr;
    struct SolidSyslogAddress* addr             = nullptr;

    void setup() override
    {
        addr = AddressFake_Get();
        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&rng);
        const unsigned char pers[] = "mbedtls-integration-test";
        mbedtls_ctr_drbg_seed(&rng, mbedtls_entropy_func, &entropy, pers, sizeof(pers) - 1U);

        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        /* Bound both sides' blocking recv to 5s. The OpenSSL integration
         * tests use an in-memory BIO pair that never blocks; our socketpair
         * harness inherently can deadlock on negative-path handshakes (one
         * side waits for a message the other won't send). The cap matches
         * the production handshake retry budget — generous for any real
         * handshake (sub-second), tight enough that a stuck test fails fast. */
        struct timeval rcvTimeout = {5, 0};
        setsockopt(fds[0], SOL_SOCKET, SO_RCVTIMEO, &rcvTimeout, sizeof(rcvTimeout));
        setsockopt(fds[1], SOL_SOCKET, SO_RCVTIMEO, &rcvTimeout, sizeof(rcvTimeout));

        struct MbedTlsTestCertConfig caConfig = {};
        caConfig.SubjectName = TEST_CA_SUBJECT;
        caConfig.IsCa = 1;
        MbedTlsTestCert_Create(&caConfig, &trustedCa, &rng);

        struct MbedTlsTestCertConfig serverConfig = {};
        serverConfig.SubjectName = TEST_SERVER_SUBJECT;
        serverConfig.SubjectAltDns = TEST_SERVER_HOSTNAME;
        serverConfig.IsCa = 0;
        serverConfig.Issuer = &trustedCa;
        MbedTlsTestCert_Create(&serverConfig, &serverCert, &rng);
    }

    void teardown() override
    {
        if (tlsStream != nullptr)
        {
            SolidSyslogMbedTlsStream_Destroy(tlsStream);
        }
        if (clientTransport != nullptr)
        {
            SocketStream_Destroy(clientTransport);
        }
        if (server != nullptr)
        {
            MbedTlsTestServer_Destroy(server);
        }
        MbedTlsTestCert_Destroy(&serverCert);
        MbedTlsTestCert_Destroy(&trustedCa);
        mbedtls_ctr_drbg_free(&rng);
        mbedtls_entropy_free(&entropy);
    }

    struct SolidSyslogStream* StartServerWithCert(const struct MbedTlsTestCert* cert)
    {
        return StartServerRequiringClientCa(cert, nullptr);
    }

    /* mTLS variant: server requires + verifies a client cert against
     * `trustedClientCa`. Passing nullptr behaves like StartServerWithCert. */
    // NOLINTBEGIN(bugprone-easily-swappable-parameters) -- both args are MbedTlsTestCert by domain; the function name distinguishes their roles
    struct SolidSyslogStream* StartServerRequiringClientCa(
        const struct MbedTlsTestCert* cert,
        const struct MbedTlsTestCert* trustedClientCa
    )
    // NOLINTEND(bugprone-easily-swappable-parameters)
    {
        struct MbedTlsTestServerConfig serverConfig = {};
        serverConfig.ServerFd = fds[1];
        serverConfig.ServerCert = cert;
        serverConfig.Rng = &rng;
        serverConfig.TrustedClientCa = trustedClientCa;
        server = MbedTlsTestServer_Create(&serverConfig);
        clientTransport = SocketStream_Create(fds[0]);
        return clientTransport;
    }

    /* Build a CA + leaf-cert pair signed by it, for the per-test mTLS
     * material. Both outputs must be MbedTlsTestCert_Destroy'd by the
     * test body before it returns. */
    void CreateClientIdentitySignedBy(
        const struct MbedTlsTestCert* signingCa,
        struct MbedTlsTestCert* outClientCert
    )
    {
        struct MbedTlsTestCertConfig leafConfig = {};
        leafConfig.SubjectName = "CN=solidsyslog-test-client";
        leafConfig.IsCa = 0;
        leafConfig.Issuer = signingCa;
        MbedTlsTestCert_Create(&leafConfig, outClientCert, &rng);
    }

    /* Common config wiring used by every integration test: transport, sleep,
     * fixture-owned DRBG, and the trusted-CA / hostname pair built in setup().
     * Per-test tweaks (e.g. swapping the CA chain to test rejection, or
     * adding ClientCertChain + ClientKey for mTLS) overlay onto the returned
     * struct before passing it to SolidSyslogMbedTlsStream_Create. */
    struct SolidSyslogMbedTlsStreamConfig BuildBaseConfig(struct SolidSyslogStream* transport)
    {
        struct SolidSyslogMbedTlsStreamConfig cfg = {};
        cfg.Transport = transport;
        cfg.Sleep = NoOpSleep;
        cfg.Rng = &rng;
        cfg.CaChain = &trustedCa.Cert;
        cfg.ServerName = TEST_SERVER_HOSTNAME;
        return cfg;
    }
};

// clang-format on

TEST(SolidSyslogMbedTlsStreamIntegration, HandshakeSucceedsWhenServerCertSignedByTrustedCaAndHostnameMatches)

{
    struct SolidSyslogStream* transport = StartServerWithCert(&serverCert);
    struct SolidSyslogMbedTlsStreamConfig config = BuildBaseConfig(transport);
    tlsStream = SolidSyslogMbedTlsStream_Create(&config);

    bool opened = SolidSyslogStream_Open(tlsStream, addr);

    CHECK_TRUE_TEXT(opened, "client-side Open (incl. handshake) should succeed against a trusted server");
    CHECK_TRUE_TEXT(
        MbedTlsTestServer_JoinAndHandshakeSucceeded(server),
        "server-side handshake should mirror the client's success"
    );
}

TEST(SolidSyslogMbedTlsStreamIntegration, HandshakeFailsWhenServerCertSignedByUntrustedCa)

{
    /* Trust an unrelated CA: we hand the *client* a different CA chain than
     * the one that signed the server's cert, so the chain validation fails. */
    struct MbedTlsTestCert untrustedCa = {};
    struct MbedTlsTestCertConfig untrustedConfig = {};
    untrustedConfig.SubjectName = "CN=Wrong Root CA";
    untrustedConfig.IsCa = 1;
    MbedTlsTestCert_Create(&untrustedConfig, &untrustedCa, &rng);

    struct SolidSyslogStream* transport = StartServerWithCert(&serverCert);
    struct SolidSyslogMbedTlsStreamConfig config = BuildBaseConfig(transport);
    config.CaChain = &untrustedCa.Cert;
    tlsStream = SolidSyslogMbedTlsStream_Create(&config);

    bool opened = SolidSyslogStream_Open(tlsStream, addr);

    CHECK_FALSE_TEXT(opened, "client-side handshake must fail when the server cert chains to an untrusted CA");

    MbedTlsTestCert_Destroy(&untrustedCa);
}

TEST(SolidSyslogMbedTlsStreamIntegration, HandshakeFailsWhenServerNameDoesNotMatchCert)

{
    struct SolidSyslogStream* transport = StartServerWithCert(&serverCert);
    struct SolidSyslogMbedTlsStreamConfig config = BuildBaseConfig(transport);
    config.ServerName = "wrong-host.example.com"; /* server cert has SAN syslog.example.com */
    tlsStream = SolidSyslogMbedTlsStream_Create(&config);

    bool opened = SolidSyslogStream_Open(tlsStream, addr);

    CHECK_FALSE_TEXT(opened, "client-side handshake must fail when ServerName does not match the cert's SAN");
}

TEST(SolidSyslogMbedTlsStreamIntegration, MutualTlsHandshakeSucceedsWithClientCertSignedByTrustedCa)

{
    /* Build per-test mTLS material: a client CA + a leaf cert signed by it.
     * Server is told to require + verify client certs against this CA. */
    struct MbedTlsTestCert clientCa = {};
    struct MbedTlsTestCertConfig clientCaConfig = {};
    clientCaConfig.SubjectName = "CN=Test Client CA";
    clientCaConfig.IsCa = 1;
    MbedTlsTestCert_Create(&clientCaConfig, &clientCa, &rng);
    struct MbedTlsTestCert clientCert = {};
    CreateClientIdentitySignedBy(&clientCa, &clientCert);

    struct SolidSyslogStream* transport = StartServerRequiringClientCa(&serverCert, &clientCa);
    struct SolidSyslogMbedTlsStreamConfig config = BuildBaseConfig(transport);
    config.ClientCertChain = &clientCert.Cert;
    config.ClientKey = &clientCert.Key;
    tlsStream = SolidSyslogMbedTlsStream_Create(&config);

    bool opened = SolidSyslogStream_Open(tlsStream, addr);

    CHECK_TRUE_TEXT(opened, "client-side mTLS Open should succeed against a server trusting the client CA");
    CHECK_TRUE_TEXT(
        MbedTlsTestServer_JoinAndHandshakeSucceeded(server),
        "server-side mTLS handshake should mirror the client's success"
    );

    MbedTlsTestCert_Destroy(&clientCert);
    MbedTlsTestCert_Destroy(&clientCa);
}

TEST(SolidSyslogMbedTlsStreamIntegration, MutualTlsHandshakeRejectedWhenClientSendsNoCert)

{
    /* Server requires a client cert but the integrator hasn't opted in to
     * mTLS — ClientCertChain / ClientKey are NULL. Server-side verify must
     * fail and the client's Open must return false. */
    struct MbedTlsTestCert clientCa = {};
    struct MbedTlsTestCertConfig clientCaConfig = {};
    clientCaConfig.SubjectName = "CN=Test Client CA";
    clientCaConfig.IsCa = 1;
    MbedTlsTestCert_Create(&clientCaConfig, &clientCa, &rng);

    struct SolidSyslogStream* transport = StartServerRequiringClientCa(&serverCert, &clientCa);
    struct SolidSyslogMbedTlsStreamConfig config = BuildBaseConfig(transport);
    tlsStream = SolidSyslogMbedTlsStream_Create(&config);

    bool opened = SolidSyslogStream_Open(tlsStream, addr);

    CHECK_FALSE_TEXT(opened, "mTLS handshake must fail when the client does not present a cert");

    MbedTlsTestCert_Destroy(&clientCa);
}

TEST(SolidSyslogMbedTlsStreamIntegration, MutualTlsHandshakeRejectedWhenClientCertSignedByUntrustedCa)

{
    /* Client cert is signed by a CA the server doesn't trust. Server-side
     * chain validation fails and Open returns false on the client. */
    struct MbedTlsTestCert trustedClientCa = {};
    struct MbedTlsTestCertConfig trustedConfig = {};
    trustedConfig.SubjectName = "CN=Trusted Client CA";
    trustedConfig.IsCa = 1;
    MbedTlsTestCert_Create(&trustedConfig, &trustedClientCa, &rng);

    struct MbedTlsTestCert untrustedClientCa = {};
    struct MbedTlsTestCertConfig untrustedConfig = {};
    untrustedConfig.SubjectName = "CN=Untrusted Client CA";
    untrustedConfig.IsCa = 1;
    MbedTlsTestCert_Create(&untrustedConfig, &untrustedClientCa, &rng);

    /* Client cert is signed by the *untrusted* CA — server only trusts
     * trustedClientCa, so verify will reject this chain. */
    struct MbedTlsTestCert clientCert = {};
    CreateClientIdentitySignedBy(&untrustedClientCa, &clientCert);

    struct SolidSyslogStream* transport = StartServerRequiringClientCa(&serverCert, &trustedClientCa);
    struct SolidSyslogMbedTlsStreamConfig config = BuildBaseConfig(transport);
    config.ClientCertChain = &clientCert.Cert;
    config.ClientKey = &clientCert.Key;
    tlsStream = SolidSyslogMbedTlsStream_Create(&config);

    bool opened = SolidSyslogStream_Open(tlsStream, addr);

    CHECK_FALSE_TEXT(opened, "mTLS handshake must fail when the client cert chains to an untrusted CA");

    MbedTlsTestCert_Destroy(&clientCert);
    MbedTlsTestCert_Destroy(&untrustedClientCa);
    MbedTlsTestCert_Destroy(&trustedClientCa);
}

TEST(SolidSyslogMbedTlsStreamIntegration, BinaryLinksAgainstRealLibMbedTls)

{
    /* mbedtls_version_get_number() is a constant, side-effect-free symbol
     * present in every mbedTLS build — a successful link plus a return
     * value matching the expected major version (3.x) confirms the
     * integration scaffold pulls in the real library, not a fake. */
    const unsigned int major = (mbedtls_version_get_number() >> 24) & 0xFFU;
    LONGS_EQUAL(3, major);
}

/* -------------------------------------------------------------------------
 * Session resumption (S26.04). Drives two consecutive handshakes on the SAME
 * SolidSyslogMbedTlsStream instance — the library's real Close -> Open
 * fail-fast reconnect — against a ticket-issuing server. The observable is
 * server-side: mbedTLS has no client-side SSL_session_reused(), so the server
 * wraps its ticket-parse callback to report when a presented ticket was
 * accepted. A reconnecting transport feeds the stream a fresh socketpair on
 * each Open; the TLS layer is still wired straight to the oracle (no
 * production TCP stream in the loop).
 * ------------------------------------------------------------------------- */

namespace
{
void SetRecvTimeout(int fd, int seconds)
{
    struct timeval rcvTimeout = {seconds, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &rcvTimeout, sizeof(rcvTimeout));
}
} // namespace

// clang-format off
TEST_GROUP(SolidSyslogMbedTlsStreamResumption)
{
    mbedtls_entropy_context    entropy         = {};
    mbedtls_ctr_drbg_context   rng             = {};
    int                        clientFds[2]    = {-1, -1};
    int                        serverFds[2]    = {-1, -1};
    struct MbedTlsTestCert     trustedCa       = {};
    struct MbedTlsTestCert     serverCert      = {};
    struct MbedTlsResumptionServer* server     = nullptr;
    struct SolidSyslogStream*  transport       = nullptr;
    struct SolidSyslogStream*  tlsStream       = nullptr;
    struct SolidSyslogAddress* addr            = nullptr;

    void setup() override
    {
        addr = AddressFake_Get();
        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&rng);
        const unsigned char pers[] = "mbedtls-resumption-test";
        mbedtls_ctr_drbg_seed(&rng, mbedtls_entropy_func, &entropy, pers, sizeof(pers) - 1U);

        /* Two pre-connected socketpairs: one per handshake. The reconnecting
         * transport consumes the client ends in order; the server thread
         * services the server ends in order. No TCP ports -> deterministic. */
        int pairA[2] = {-1, -1};
        int pairB[2] = {-1, -1};
        socketpair(AF_UNIX, SOCK_STREAM, 0, pairA);
        socketpair(AF_UNIX, SOCK_STREAM, 0, pairB);
        clientFds[0] = pairA[0];
        clientFds[1] = pairB[0];
        serverFds[0] = pairA[1];
        serverFds[1] = pairB[1];
        for (int fd : {clientFds[0], clientFds[1], serverFds[0], serverFds[1]})
        {
            SetRecvTimeout(fd, 5);
        }

        struct MbedTlsTestCertConfig caConfig = {};
        caConfig.SubjectName = TEST_CA_SUBJECT;
        caConfig.IsCa = 1;
        MbedTlsTestCert_Create(&caConfig, &trustedCa, &rng);

        struct MbedTlsTestCertConfig serverConfig = {};
        serverConfig.SubjectName = TEST_SERVER_SUBJECT;
        serverConfig.SubjectAltDns = TEST_SERVER_HOSTNAME;
        serverConfig.IsCa = 0;
        serverConfig.Issuer = &trustedCa;
        MbedTlsTestCert_Create(&serverConfig, &serverCert, &rng);
    }

    void teardown() override
    {
        /* tlsStream first: its Destroy drives the transport Close that closes
         * the current client fd, so it must run before the transport is freed. */
        if (tlsStream != nullptr)
        {
            SolidSyslogMbedTlsStream_Destroy(tlsStream);
        }
        if (transport != nullptr)
        {
            ReconnectingSocketStream_Destroy(transport);
        }
        if (server != nullptr)
        {
            MbedTlsResumptionServer_Destroy(server);
        }
        MbedTlsTestCert_Destroy(&serverCert);
        MbedTlsTestCert_Destroy(&trustedCa);
        mbedtls_ctr_drbg_free(&rng);
        mbedtls_entropy_free(&entropy);
    }

    struct SolidSyslogMbedTlsStreamConfig BuildBaseConfig(struct SolidSyslogStream* clientTransport)
    {
        struct SolidSyslogMbedTlsStreamConfig cfg = {};
        cfg.Transport = clientTransport;
        cfg.Sleep = NoOpSleep;
        cfg.Rng = &rng;
        cfg.CaChain = &trustedCa.Cert;
        cfg.ServerName = TEST_SERVER_HOSTNAME;
        return cfg;
    }

    struct MbedTlsResumptionServer* StartServer(bool rotateTicketKey)
    {
        struct MbedTlsResumptionServerConfig serverConfig = {};
        serverConfig.ServerFds[0] = serverFds[0];
        serverConfig.ServerFds[1] = serverFds[1];
        serverConfig.ServerCert = &serverCert;
        serverConfig.Rng = &rng;
        serverConfig.RotateTicketKeyBetweenConnections = rotateTicketKey;
        return MbedTlsResumptionServer_Create(&serverConfig);
    }

    /* One Open -> Send -> (the test closes if reconnecting) cycle on the same
     * stream. Returns whether the Open and Send both succeeded. */
    bool ConnectAndDeliver(const char* payload) const
    {
        bool opened = SolidSyslogStream_Open(tlsStream, addr);
        bool sent = opened && SolidSyslogStream_Send(tlsStream, payload, 8U);
        return opened && sent;
    }
};

// clang-format on

TEST(SolidSyslogMbedTlsStreamResumption, SameStreamReconnectResumesTheTlsSession)

{
    transport = ReconnectingSocketStream_Create(clientFds, 2);
    struct SolidSyslogMbedTlsStreamConfig config = BuildBaseConfig(transport);
    tlsStream = SolidSyslogMbedTlsStream_Create(&config);
    server = StartServer(/*rotateTicketKey=*/false);

    CHECK_TRUE_TEXT(ConnectAndDeliver("<13>one\n"), "first (full) handshake + send should succeed");
    SolidSyslogStream_Close(tlsStream); /* the fail-fast reconnect the library performs in production */
    CHECK_TRUE_TEXT(ConnectAndDeliver("<13>two\n"), "second (resumed) handshake + send should succeed");

    MbedTlsResumptionServer_Join(server);
    CHECK_TRUE_TEXT(
        MbedTlsResumptionServer_BothHandshakesSucceeded(server),
        "both server-side handshakes should complete"
    );
    CHECK_TRUE_TEXT(MbedTlsResumptionServer_BothMessagesDelivered(server), "a record should arrive on each connection");
    CHECK_TRUE_TEXT(
        MbedTlsResumptionServer_SecondHandshakeResumed(server),
        "the second handshake must present a ticket the server accepts (abbreviated handshake)"
    );
}

TEST(SolidSyslogMbedTlsStreamResumption, NonResumingPeerFallsBackToFullHandshakeAndStillDelivers)

{
    transport = ReconnectingSocketStream_Create(clientFds, 2);
    struct SolidSyslogMbedTlsStreamConfig config = BuildBaseConfig(transport);
    tlsStream = SolidSyslogMbedTlsStream_Create(&config);
    /* Rotate the ticket key between connections so the client's saved ticket
     * no longer decrypts — the resume offer is rejected. */
    server = StartServer(/*rotateTicketKey=*/true);

    CHECK_TRUE_TEXT(ConnectAndDeliver("<13>one\n"), "first handshake + send should succeed");
    SolidSyslogStream_Close(tlsStream);
    CHECK_TRUE_TEXT(ConnectAndDeliver("<13>two\n"), "second handshake + send should still succeed");

    MbedTlsResumptionServer_Join(server);
    CHECK_TRUE_TEXT(MbedTlsResumptionServer_BothHandshakesSucceeded(server), "both handshakes should complete");
    CHECK_TRUE_TEXT(
        MbedTlsResumptionServer_BothMessagesDelivered(server),
        "delivery must survive the full-handshake fallback"
    );
    CHECK_FALSE_TEXT(
        MbedTlsResumptionServer_SecondHandshakeResumed(server),
        "with a rotated ticket key the second handshake must NOT resume"
    );
}
