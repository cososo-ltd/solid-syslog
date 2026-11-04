#include "CppUTest/TestHarness.h"

extern "C"
{
#ifndef WOLFSSL_USER_SETTINGS
#include <wolfssl/options.h>
#endif
#include <sys/socket.h>
#include <sys/time.h>
#include <wolfssl/version.h>

#include "AddressFake.h"
#include "SocketStream.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogWolfSslCredentialsDefinition.h"
#include "SolidSyslogWolfSslPemBufferCredentials.h"
#include "SolidSyslogWolfSslStream.h"
#include "SolidSyslogWolfSslStreamErrors.h"
#include "WolfSslTestLibrary.h"
#include "WolfSslTestMaterial.h"
#include "WolfSslTestServer.h"
}

#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogTlsStreamCategories.h"
#include "SolidSyslogTlsStreamErrors.h"

namespace
{
constexpr const char* TEST_SERVER_HOSTNAME = "syslog.example.com";

void NoOpSleep(uint32_t milliseconds)
{
    (void) milliseconds;
}

/* The fixture asserts the material is there, so a missing generator run reads
   as one failure and not as a refused handshake. */
void LoadMaterial(const char* name, struct WolfSslTestMaterial* out)
{
    CHECK_TRUE_TEXT(
        WolfSslTestMaterial_Load(name, out),
        "test material missing - was the configure-time generator run?"
    );
}
} // namespace

/* This suite links no ErrorHandlerFake - that is the unit executable's, and this
 * one builds against the real libwolfssl. Capturing the last event directly is
 * enough to pin which code a real fault produces. */
static int CapturedErrorCount;
static struct SolidSyslogErrorEvent LastCapturedError;

static void CaptureError(void* context, const struct SolidSyslogErrorEvent* event)
{
    (void) context;
    CapturedErrorCount++;
    LastCapturedError = *event;
}

/* Pins a refused handshake to the check that refused it, against the real
 * libwolfssl rather than the fake's canned verdict. */
#define CHECK_REFUSAL_REPORTED(expectedCode)                                                           \
    {                                                                                                  \
        LONGS_EQUAL(1, CapturedErrorCount);                                                            \
        LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, LastCapturedError.Severity);                           \
        POINTERS_EQUAL(&SolidSyslogWolfSslStreamErrorSource, LastCapturedError.Source);                \
        UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED, LastCapturedError.Category); \
        LONGS_EQUAL((expectedCode), LastCapturedError.Detail);                                         \
    }

/* A connection the stream made without anything to say about it. */
#define CHECK_OPENS_WITHOUT_A_REPORT(stream, address)            \
    {                                                            \
        CHECK_TRUE(SolidSyslogStream_Open((stream), (address))); \
        LONGS_EQUAL(0, CapturedErrorCount);                      \
    }

// clang-format off
/* What the scenario under construction wants the connection made with, held per
 * test on the fixture and reached through ProfileContext. */
struct IntegrationProfileValues
{
    const char* ServerName;
};

static void IntegrationProfile(struct SolidSyslogWolfSslProfile* profile, void* context)
{
    const auto* values  = static_cast<const struct IntegrationProfileValues*>(context);
    profile->ServerName = values->ServerName;
}

TEST_GROUP(SolidSyslogWolfSslStreamIntegration)
{
    struct IntegrationProfileValues profileValues = {};
    int                        fds[2]           = {-1, -1};
    struct WolfSslTestMaterial            trustedCa        = {};
    struct WolfSslTestMaterial            serverCert       = {};
    struct WolfSslTestMaterial            serverKey        = {};
    struct WolfSslTestServer*  server           = nullptr;
    struct SolidSyslogStream*  clientTransport  = nullptr;
    struct SolidSyslogStream*  tlsStream        = nullptr;
    struct SolidSyslogWolfSslPemBufferCredentialsConfig credsConfig = {};
    struct SolidSyslogWolfSslCredentials* credentials = nullptr;
    struct SolidSyslogAddress* addr             = nullptr;

    void setup() override
    {
        WolfSslTestLibrary_Init();
        addr = AddressFake_Get();
        CapturedErrorCount = 0;
        LastCapturedError = {};
        SolidSyslog_SetErrorHandler(CaptureError, nullptr);

        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        /* Bound both sides' blocking recv, so a negative-path handshake that
         * leaves one side waiting for a message the other never sends fails
         * fast rather than hanging the suite. */
        struct timeval rcvTimeout = {5, 0};
        setsockopt(fds[0], SOL_SOCKET, SO_RCVTIMEO, &rcvTimeout, sizeof(rcvTimeout));
        setsockopt(fds[1], SOL_SOCKET, SO_RCVTIMEO, &rcvTimeout, sizeof(rcvTimeout));

        LoadMaterial("ca.pem", &trustedCa);
    }

    void teardown() override
    {
        SolidSyslog_SetErrorHandler(nullptr, nullptr);
        if (tlsStream != nullptr)
        {
            SolidSyslogWolfSslStream_Destroy(tlsStream);
        }
        if (credentials != nullptr)
        {
            SolidSyslogWolfSslPemBufferCredentials_Destroy(credentials);
        }
        if (clientTransport != nullptr)
        {
            SocketStream_Destroy(clientTransport);
        }
        if (server != nullptr)
        {
            WolfSslTestServer_Destroy(server);
        }
        WolfSslTestLibrary_Cleanup();
    }

    /* A server presenting the named certificate, and the transport to it. */
    struct SolidSyslogStream* StartServerPresenting(const char* certificate)
    {
        char keyName[64];
        WolfSslTestMaterial_KeyNameFor(certificate, keyName, sizeof(keyName));
        LoadMaterial(certificate, &serverCert);
        LoadMaterial(keyName, &serverKey);
        struct WolfSslTestServerConfig serverConfig = {};
        serverConfig.ServerFd = fds[1];
        serverConfig.CertificatePath = serverCert.Path;
        serverConfig.KeyPath = serverKey.Path;
        server = WolfSslTestServer_Create(&serverConfig);
        clientTransport = SocketStream_Create(fds[0]);
        return clientTransport;
    }

    /* The common wiring: the transport, a no-op sleep, the fixture's trust
     * anchors, and the name the profile expects - or NULL / "" for the two
     * ways of not naming one. Per-test tweaks overlay onto either struct before
     * CreateTlsStream. */
    struct SolidSyslogWolfSslStreamConfig BuildConfigExpecting(
        struct SolidSyslogStream* transport,
        const char* serverName
    )
    {
        credsConfig = {};
        credsConfig.CaPem.Bytes = trustedCa.Bytes;
        credsConfig.CaPem.Length = trustedCa.Length;

        struct SolidSyslogWolfSslStreamConfig cfg = {};
        cfg.Transport = transport;
        cfg.Sleep = NoOpSleep;
        profileValues.ServerName = serverName;
        cfg.Profile = IntegrationProfile;
        cfg.ProfileContext = &profileValues;
        return cfg;
    }

    struct SolidSyslogWolfSslStreamConfig BuildConfigAgainst(const char* certificate, const char* serverName)
    {
        return BuildConfigExpecting(StartServerPresenting(certificate), serverName);
    }

    struct SolidSyslogStream* CreateTlsStream(struct SolidSyslogWolfSslStreamConfig* cfg)
    {
        credentials = SolidSyslogWolfSslPemBufferCredentials_Create(&credsConfig);
        cfg->Credentials = credentials;
        return SolidSyslogWolfSslStream_Create(cfg);
    }

    /* Open against the named certificate expecting the named peer. */
    bool OpenAgainst(const char* certificate, const char* serverName)
    {
        struct SolidSyslogWolfSslStreamConfig config = BuildConfigAgainst(certificate, serverName);
        tlsStream = CreateTlsStream(&config);
        return SolidSyslogStream_Open(tlsStream, addr);
    }
};

// clang-format on

TEST(SolidSyslogWolfSslStreamIntegration, BinaryLinksAgainstRealLibWolfSsl)
{
    STRCMP_EQUAL(LIBWOLFSSL_VERSION_STRING, WolfSslTestLibrary_Version());
}

TEST(SolidSyslogWolfSslStreamIntegration, HandshakeSucceedsWhenServerCertSignedByTrustedCaAndHostnameMatches)
{
    struct SolidSyslogWolfSslStreamConfig config = BuildConfigAgainst("server.pem", TEST_SERVER_HOSTNAME);
    tlsStream = CreateTlsStream(&config);

    CHECK_OPENS_WITHOUT_A_REPORT(tlsStream, addr);
    SolidSyslogStream_Close(tlsStream);
    CHECK_TRUE_TEXT(
        WolfSslTestServer_JoinAndHandshakeSucceeded(server),
        "server-side handshake should mirror the client's success"
    );
}

TEST(SolidSyslogWolfSslStreamIntegration, ARecordSentAfterTheHandshakeReachesTheServer)
{
    struct SolidSyslogWolfSslStreamConfig config = BuildConfigAgainst("server.pem", TEST_SERVER_HOSTNAME);
    tlsStream = CreateTlsStream(&config);
    SolidSyslogStream_Open(tlsStream, addr);
    /* A send first reads what the peer has sent, as the production transports
       answer would-block; the handshake above ran blocking so it needed no
       poll loop. */
    SocketStream_SetNonBlocking(clientTransport);

    CHECK_TRUE(SolidSyslogStream_Send(tlsStream, "<134>1 - - - - - - hello", 24U));
    SolidSyslogStream_Close(tlsStream);

    STRCMP_EQUAL("<134>1 - - - - - - hello", WolfSslTestServer_Received(server));
}

/* No session exists before Open, and the stream hands the library a NULL one
 * rather than guarding it: wolfSSL refuses that as a bad argument, which the
 * read maps to a closed session. Pinned here because only the real library can
 * prove what it does with a NULL. */
TEST(SolidSyslogWolfSslStreamIntegration, SendBeforeOpenIsRefusedAndReachesNoServer)
{
    struct SolidSyslogWolfSslStreamConfig config = BuildConfigAgainst("server.pem", TEST_SERVER_HOSTNAME);
    tlsStream = CreateTlsStream(&config);

    CHECK_FALSE(SolidSyslogStream_Send(tlsStream, "x", 1U));

    LONGS_EQUAL(0, CapturedErrorCount);
    /* Closing the client end is what lets the server's accept return. */
    SolidSyslogStream_Close(clientTransport);
    CHECK_FALSE(WolfSslTestServer_JoinAndHandshakeSucceeded(server));
}

TEST(SolidSyslogWolfSslStreamIntegration, HandshakeFailsWhenServerCertSignedByUntrustedCa)
{
    CHECK_FALSE(OpenAgainst("server-untrusted.pem", TEST_SERVER_HOSTNAME));

    CHECK_REFUSAL_REPORTED(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_UNTRUSTED);
}

TEST(SolidSyslogWolfSslStreamIntegration, HandshakeFailsWhenServerNameDoesNotMatchCert)
{
    CHECK_FALSE(OpenAgainst("server.pem", "wrong-host.example.com"));

    CHECK_REFUSAL_REPORTED(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_NAME_MISMATCHED);
}

/* The same refusal as the name above, for an expected identity written as an
 * address the certificate does not carry. The portable detail must not depend
 * on which form the integrator used. */
TEST(SolidSyslogWolfSslStreamIntegration, HandshakeFailsWhenTheExpectedAddressIsNotInTheCert)
{
    CHECK_FALSE(OpenAgainst("server.pem", "127.0.0.1"));

    CHECK_REFUSAL_REPORTED(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_NAME_MISMATCHED);
}

/* An address the certificate does carry, as an iPAddress entry. Only a wolfSSL
 * built with WOLFSSL_IP_ALT_NAME matches an expected address against those; a
 * bare build matches the text as a DNS name, which an iPAddress entry never
 * satisfies, and refuses. The same source is compiled against both, so each
 * executable asserts the behaviour of the build it links. */
#if defined(WOLFSSL_IP_ALT_NAME)
TEST(SolidSyslogWolfSslStreamIntegration, AnExpectedAddressTheCertificateCarriesIsMatchedByABuildThatMatchesAddresses)
{
    CHECK_TRUE(OpenAgainst("server-ip.pem", "127.0.0.1"));

    LONGS_EQUAL(0, CapturedErrorCount);
}
#else
TEST(SolidSyslogWolfSslStreamIntegration, AnExpectedAddressTheCertificateCarriesIsRefusedByABuildThatCannotMatchOne)
{
    CHECK_FALSE(OpenAgainst("server-ip.pem", "127.0.0.1"));

    CHECK_REFUSAL_REPORTED(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_NAME_MISMATCHED);
}
#endif

/* No trust anchors and no pinned fingerprint: nothing authorises the peer, so
 * the connection stops before the handshake rather than reaching a collector
 * this stream cannot identify. */
TEST(SolidSyslogWolfSslStreamIntegration, OpenFailsWhenNothingAuthorisesThePeer)
{
    struct SolidSyslogWolfSslStreamConfig config = BuildConfigAgainst("server.pem", TEST_SERVER_HOSTNAME);
    credsConfig.CaPem.Bytes = nullptr;
    credsConfig.CaPem.Length = 0U;
    tlsStream = CreateTlsStream(&config);

    CHECK_FALSE(SolidSyslogStream_Open(tlsStream, addr));

    LONGS_EQUAL(1, CapturedErrorCount);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, LastCapturedError.Severity);
    POINTERS_EQUAL(&SolidSyslogWolfSslStreamErrorSource, LastCapturedError.Source);
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_BAD_CONFIG, LastCapturedError.Category);
    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_NO_PEER_AUTHORISATION, LastCapturedError.Detail);
}

TEST(SolidSyslogWolfSslStreamIntegration, HandshakeFailsWhenServerCertHasExpired)
{
    CHECK_FALSE(OpenAgainst("server-expired.pem", TEST_SERVER_HOSTNAME));

    CHECK_REFUSAL_REPORTED(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_EXPIRED);
}

TEST(SolidSyslogWolfSslStreamIntegration, HandshakeFailsWhenServerCertIsNotYetValid)
{
    CHECK_FALSE(OpenAgainst("server-notyetvalid.pem", TEST_SERVER_HOSTNAME));

    CHECK_REFUSAL_REPORTED(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_NOT_YET_VALID);
}

TEST(SolidSyslogWolfSslStreamIntegration, HandshakeSucceedsChainOnlyWithNoServerNameAndWarns)
{
    CHECK_TRUE(OpenAgainst("server-wrongname.pem", nullptr));

    LONGS_EQUAL(1, CapturedErrorCount);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, LastCapturedError.Severity);
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_BAD_CONFIG, LastCapturedError.Category);
    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_SERVER_NAME_NOT_SET, LastCapturedError.Detail);
}

TEST(SolidSyslogWolfSslStreamIntegration, HandshakeSucceedsChainOnlyWhenServerNameIsEmpty)
{
    CHECK_TRUE(OpenAgainst("server-wrongname.pem", ""));

    LONGS_EQUAL(0, CapturedErrorCount);
}

/* The order the contract fixes, against what the library actually raises. */
TEST(SolidSyslogWolfSslStreamIntegration, AnUntrustedChainIsNamedBeforeANameThatDoesNotMatch)
{
    CHECK_FALSE(OpenAgainst("server-untrusted.pem", "wrong-host.example.com"));

    CHECK_REFUSAL_REPORTED(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_UNTRUSTED);
}

TEST(SolidSyslogWolfSslStreamIntegration, AnUntrustedChainIsNamedBeforeTheDatesOnIt)
{
    CHECK_FALSE(OpenAgainst("server-untrusted-expired.pem", TEST_SERVER_HOSTNAME));

    CHECK_REFUSAL_REPORTED(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_UNTRUSTED);
}

/* The contract names the name ahead of the dates. wolfSSL cannot: a date
 * failure on the leaf is fatal before its name is checked, so the name is
 * never raised and the stream has nothing to put first. The platform page
 * records the shortfall; this pins it, so a wolfSSL that starts checking the
 * name regardless is noticed. */
TEST(
    SolidSyslogWolfSslStreamIntegration,
    ACertificateThatHasExpiredIsNamedForItsDatesEvenWhereItsNameDoesNotMatchEither
)
{
    CHECK_FALSE(OpenAgainst("server-wrongname-expired.pem", TEST_SERVER_HOSTNAME));

    CHECK_REFUSAL_REPORTED(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_EXPIRED);
}

/* A key below the BCP 195 section 4.5 floor fails the library's own key-size
 * check on the certificate, which is a check on the chain and names the
 * certificate as the thing to replace. */
TEST(SolidSyslogWolfSslStreamIntegration, AKeyBelowTheFloorIsRefusedAsAnUntrustedCertificate)
{
    CHECK_FALSE(OpenAgainst("server-weak.pem", TEST_SERVER_HOSTNAME));

    CHECK_REFUSAL_REPORTED(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_UNTRUSTED);
}
