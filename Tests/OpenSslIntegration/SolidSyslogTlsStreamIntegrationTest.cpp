#include <openssl/err.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "BioPairStream.h"
#include "SolidSyslogAddress.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogTlsStream.h"
#include "TlsTestCert.h"
#include "TlsTestServer.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(TlsStreamIntegration)
{
    struct TlsTestCert                cert           = {};
    struct TlsTestCert                clientCa       = {};
    struct TlsTestCert                clientCert     = {};
    struct TlsTestServer*             server         = nullptr;
    struct SolidSyslogStream*         transport      = nullptr;
    struct SolidSyslogTlsStreamConfig tlsConfig      = {};
    SolidSyslogTlsStreamStorage       tlsStreamStorage{};
    struct SolidSyslogStream*         tlsStream      = nullptr;
    SolidSyslogAddressStorage         addrStorage    = {};
    struct SolidSyslogAddress*        addr           = nullptr;
    char                              caPath[64]     = {};
    char                              clientCertPath[64] = {};
    char                              clientKeyPath[64]  = {};

    void setup() override
    {
        addr = SolidSyslogAddress_FromStorage(&addrStorage);
    }

    void teardown() override
    {
        if (tlsStream != nullptr)         { SolidSyslogTlsStream_Destroy(tlsStream); }
        if (transport != nullptr)         { BioPairStream_Destroy(transport); }
        if (server != nullptr)            { TlsTestServer_Destroy(server); }
        if (cert.cert != nullptr)         { TlsTestCert_Destroy(&cert); }
        if (clientCert.cert != nullptr)   { TlsTestCert_Destroy(&clientCert); }
        if (clientCa.cert != nullptr)     { TlsTestCert_Destroy(&clientCa); }
        if (caPath[0] != '\0')            { unlink(caPath); }
        if (clientCertPath[0] != '\0')    { unlink(clientCertPath); }
        if (clientKeyPath[0] != '\0')     { unlink(clientKeyPath); }
    }

    template <std::size_t N>
    static void makeTempFile(char (&out)[N])
    {
        static constexpr const char TEMPLATE[] = "/tmp/solidsyslog_mtls_XXXXXX";
        static_assert(sizeof(TEMPLATE) <= N, "buffer too small for mkstemp template");
        std::memcpy(out, TEMPLATE, sizeof(TEMPLATE));
        int fd = mkstemp(out);
        CHECK_TRUE(fd >= 0);
        if (fd >= 0) { close(fd); }
    }

    void buildScenario(const struct TlsTestCertConfig& certConfig,
                       const char*                     clientServerName = "localhost",
                       const struct TlsTestCert*       serverClientCa   = nullptr)
    {
        TlsTestCert_Create(&certConfig, &cert);
        makeTempFile(caPath);
        TlsTestCert_WritePemToFile(&cert, caPath);

        struct TlsTestServerConfig serverConfig = {};
        serverConfig.serverCert   = &cert;
        serverConfig.clientCaCert = serverClientCa;
        server                    = TlsTestServer_Create(&serverConfig);

        transport = BioPairStream_Create(TlsTestServer_ClientSideBio(server));
        BioPairStream_SetPump(transport, TlsTestServer_Pump, server);

        tlsConfig.transport    = transport;
        tlsConfig.caBundlePath = caPath;
        tlsConfig.serverName   = clientServerName;
        tlsStream              = SolidSyslogTlsStream_Create(&tlsStreamStorage, &tlsConfig);
    }

    /* Creates the client-side mTLS material and writes it to disk.
     * `signingCa` signs the client leaf cert. Pass `&clientCa` for the happy
     * path (server-trusted), or a separately-created throwaway CA to drive
     * the "client cert not trusted by server" scenario. */
    void stageClientIdentity(const struct TlsTestCert* signingCa)
    {
        struct TlsTestCertConfig leafConfig = {};
        leafConfig.commonName               = "solidsyslog-test-client";
        leafConfig.issuer                   = signingCa;
        TlsTestCert_Create(&leafConfig, &clientCert);

        makeTempFile(clientCertPath);
        makeTempFile(clientKeyPath);
        TlsTestCert_WritePemToFile(&clientCert, clientCertPath);
        TlsTestCert_WritePrivateKeyPemToFile(&clientCert, clientKeyPath);

        tlsConfig.clientCertChainPath = clientCertPath;
        tlsConfig.clientKeyPath       = clientKeyPath;
    }

    void createClientCa()
    {
        struct TlsTestCertConfig caConfig = {};
        caConfig.commonName               = "SolidSyslog Test Client CA";
        TlsTestCert_Create(&caConfig, &clientCa);
    }
};

// clang-format on

static const char* const LOCALHOST_SANS[] = {"localhost", nullptr};

TEST(TlsStreamIntegration, HandshakeSucceedsAgainstTrustedServerCert)
{
    struct TlsTestCertConfig certConfig = {};
    certConfig.commonName               = "localhost";
    certConfig.subjectAltDnsNames       = LOCALHOST_SANS;
    buildScenario(certConfig);

    CHECK_TRUE(SolidSyslogStream_Open(tlsStream, addr));
}

TEST(TlsStreamIntegration, HandshakeRejectedWhenServerCertIsExpired)
{
    struct TlsTestCertConfig certConfig = {};
    certConfig.commonName               = "localhost";
    certConfig.subjectAltDnsNames       = LOCALHOST_SANS;
    certConfig.notBefore                = std::time(nullptr) - 7200;
    certConfig.notAfter                 = std::time(nullptr) - 3600;
    buildScenario(certConfig);

    CHECK_FALSE(SolidSyslogStream_Open(tlsStream, addr));
}

TEST(TlsStreamIntegration, HandshakeRejectedWhenServerCertHostnameDoesNotMatch)
{
    static const char* const otherSans[] = {"someone-else.example", nullptr};
    struct TlsTestCertConfig certConfig  = {};
    certConfig.commonName                = "someone-else.example";
    certConfig.subjectAltDnsNames        = otherSans;
    buildScenario(certConfig); /* client.serverName defaults to "localhost" */

    CHECK_FALSE(SolidSyslogStream_Open(tlsStream, addr));
}

TEST(TlsStreamIntegration, HandshakeRejectedWhenClientDoesNotTrustServerCert)
{
    struct TlsTestCertConfig certConfig = {};
    certConfig.commonName               = "localhost";
    certConfig.subjectAltDnsNames       = LOCALHOST_SANS;
    buildScenario(certConfig);

    /* Overwrite the client's trust file with an unrelated self-signed cert
     * so the server's cert is no longer anchored in the trust store. The CA
     * file is loaded on Open, so this replacement takes effect for the next
     * handshake attempt. */
    struct TlsTestCertConfig untrustedConfig = {};
    untrustedConfig.commonName               = "some-other-entity.example";
    struct TlsTestCert untrusted             = {};
    TlsTestCert_Create(&untrustedConfig, &untrusted);
    TlsTestCert_WritePemToFile(&untrusted, caPath);

    CHECK_FALSE(SolidSyslogStream_Open(tlsStream, addr));

    TlsTestCert_Destroy(&untrusted);
}

TEST(TlsStreamIntegration, HandshakeRejectedWhenCipherListIsUnsupported)
{
    struct TlsTestCertConfig certConfig = {};
    certConfig.commonName               = "localhost";
    certConfig.subjectAltDnsNames       = LOCALHOST_SANS;
    tlsConfig.cipherList                = "NOT-A-REAL-CIPHER";
    buildScenario(certConfig);

    CHECK_FALSE(SolidSyslogStream_Open(tlsStream, addr));
}

/* -------------------------------------------------------------------------
 * Mutual TLS — client cert + private key (S03.09).
 * ------------------------------------------------------------------------- */

TEST(TlsStreamIntegration, MutualTlsHandshakeSucceedsWithClientCertSignedByTrustedCa)
{
    createClientCa();
    stageClientIdentity(&clientCa);

    struct TlsTestCertConfig serverCertConfig = {};
    serverCertConfig.commonName               = "localhost";
    serverCertConfig.subjectAltDnsNames       = LOCALHOST_SANS;
    buildScenario(serverCertConfig, "localhost", &clientCa);

    bool opened = SolidSyslogStream_Open(tlsStream, addr);
    if (!opened)
    {
        ERR_print_errors_fp(stderr);
    }
    CHECK_TRUE(opened);
}

TEST(TlsStreamIntegration, MutualTlsHandshakeRejectedWhenClientSendsNoCert)
{
    createClientCa();
    /* Client config intentionally leaves clientCertChainPath / clientKeyPath NULL. */

    struct TlsTestCertConfig serverCertConfig = {};
    serverCertConfig.commonName               = "localhost";
    serverCertConfig.subjectAltDnsNames       = LOCALHOST_SANS;
    buildScenario(serverCertConfig, "localhost", &clientCa);

    CHECK_FALSE(SolidSyslogStream_Open(tlsStream, addr));
}

TEST(TlsStreamIntegration, MutualTlsOpenFailsLocallyWhenClientKeyDoesNotMatchCert)
{
    createClientCa();
    stageClientIdentity(&clientCa);

    /* Overwrite the key file with an unrelated private key so
     * SSL_CTX_check_private_key should reject the pairing before any bytes
     * hit the server. */
    struct TlsTestCertConfig strayConfig = {};
    strayConfig.commonName               = "unrelated";
    struct TlsTestCert strayCert         = {};
    TlsTestCert_Create(&strayConfig, &strayCert);
    TlsTestCert_WritePrivateKeyPemToFile(&strayCert, clientKeyPath);

    struct TlsTestCertConfig serverCertConfig = {};
    serverCertConfig.commonName               = "localhost";
    serverCertConfig.subjectAltDnsNames       = LOCALHOST_SANS;
    buildScenario(serverCertConfig, "localhost", &clientCa);

    CHECK_FALSE(SolidSyslogStream_Open(tlsStream, addr));
    TlsTestCert_Destroy(&strayCert);
}

TEST(TlsStreamIntegration, MutualTlsHandshakeRejectedWhenClientCertSignedByUntrustedCa)
{
    createClientCa();

    /* Client cert is signed by a throwaway CA that the server never learns
     * about — the server's trust store only has `clientCa`. */
    struct TlsTestCertConfig untrustedCaConfig = {};
    untrustedCaConfig.commonName               = "Untrusted Client CA";
    struct TlsTestCert untrustedCa             = {};
    TlsTestCert_Create(&untrustedCaConfig, &untrustedCa);
    stageClientIdentity(&untrustedCa);

    struct TlsTestCertConfig serverCertConfig = {};
    serverCertConfig.commonName               = "localhost";
    serverCertConfig.subjectAltDnsNames       = LOCALHOST_SANS;
    buildScenario(serverCertConfig, "localhost", &clientCa);

    CHECK_FALSE(SolidSyslogStream_Open(tlsStream, addr));
    TlsTestCert_Destroy(&untrustedCa);
}
