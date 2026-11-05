#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stdint.h>
#include <string.h>

#include "AddressFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWolfSslCredentialsDefinition.h"
#include "SolidSyslogWolfSslStream.h"
#include "SolidSyslogWolfSslStreamErrors.h"
#include "StreamFake.h"
#include "WolfSslCredentialsFake.h"
#include "WolfSslFake.h"
#include "WolfSslTlsFake.h"
}

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogTlsStreamCategories.h"
#include "SolidSyslogTlsStreamErrors.h"
#include "TestUtils.h"

using namespace CososoTesting;

/* wolfSSL return values and codes as the fake answers them; the names are
   those of ssl.h and error-crypt.h. */
enum
{
    FAKE_WOLFSSL_FAILURE = 0,
    FAKE_WOLFSSL_SUCCESS = 1,
    FAKE_MEMORY_E = -125,
    FAKE_FATAL_ERROR = -313,
    FAKE_WOLFSSL_ERROR_WANT_READ = 2,
    FAKE_WOLFSSL_ERROR_WANT_WRITE = 3,
    FAKE_VERIFY = 1,
    FAKE_CA_TYPE = 6,
    FAKE_ASN_BEFORE_DATE_E = -150,
    FAKE_ASN_AFTER_DATE_E = -151,
    FAKE_ASN_SIG_CONFIRM_E = -155,
    FAKE_ASN_NO_SIGNER_E = -188,
    FAKE_DOMAIN_NAME_MISMATCH = -322,
    FAKE_IPADDR_MISMATCH = -325,
    FAKE_VERIFY_CERT_ERROR = -329,
    FAKE_WOLFSSL_ERROR_ZERO_RETURN = 6,
    FAKE_WOLFSSL_CBIO_ERR_GENERAL = -1,
    FAKE_WOLFSSL_CBIO_ERR_WANT_READ = -2
};

/* The pins the fake digest matches: wc_Hash answers 32 bytes of 0x5A under
   SHA-256 and 20 of 0x1A under SHA-1, whatever it is given. */
static const char* const TEST_SHA256_PIN = "sha-256:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:"
                                           "5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A";
static const char* const TEST_SHA1_PIN = "sha-1:1A:1A:1A:1A:1A:1A:1A:1A:1A:1A:1A:1A:1A:1A:1A:1A:1A:1A:1A:1A";
/* A pin the fake digest never matches. */
static const char* const TEST_UNMATCHED_SHA256_PIN = "sha-256:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:"
                                                     "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00";

#define CHECK_OPEN_UNWOUND_WITH_SEVERITY(transport, expectedSeverity, expectedCategory, expectedCode)      \
    {                                                                                                      \
        LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));                                              \
        LONGS_EQUAL(WolfSslTlsFake_NewCallCount(), WolfSslTlsFake_FreeCallCount());                        \
        LONGS_EQUAL(WolfSslTlsFake_CtxNewExCallCount(), WolfSslTlsFake_CtxFreeCallCount());                \
        LONGS_EQUAL(WolfSslCredentialsFake_InstallCallCount(), WolfSslCredentialsFake_ReleaseCallCount()); \
        CHECK_ERROR_REPORTED_ONCE(                                                                         \
            (expectedSeverity),                                                                            \
            &SolidSyslogWolfSslStreamErrorSource,                                                          \
            (expectedCategory),                                                                            \
            (expectedCode)                                                                                 \
        );                                                                                                 \
    }

#define CHECK_OPEN_UNWOUND_WITH_ERROR(transport, expectedCategory, expectedCode) \
    CHECK_OPEN_UNWOUND_WITH_SEVERITY(transport, SOLIDSYSLOG_SEVERITY_ERROR, expectedCategory, expectedCode)

static int NoOpSleepCallCount;
static uint32_t g_lastSleepMs;

static void NoOpSleep(uint32_t milliseconds)
{
    NoOpSleepCallCount++;
    g_lastSleepMs = milliseconds;
}

namespace
{
int FakeGetHandshakeTimeoutMs_CallCount = 0;
void* FakeGetHandshakeTimeoutMs_LastContext = nullptr;
uint32_t FakeGetHandshakeTimeoutMs_ReturnValue = SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS;

void FakeGetHandshakeTimeoutMs_Reset()
{
    FakeGetHandshakeTimeoutMs_CallCount = 0;
    FakeGetHandshakeTimeoutMs_LastContext = reinterpret_cast<void*>(0x1U); /* sentinel - overwritten on first call */
    FakeGetHandshakeTimeoutMs_ReturnValue = SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS;
}

extern "C" uint32_t FakeGetHandshakeTimeoutMs(void* context)
{
    FakeGetHandshakeTimeoutMs_CallCount++;
    FakeGetHandshakeTimeoutMs_LastContext = context;
    return FakeGetHandshakeTimeoutMs_ReturnValue;
}

/* Stands in for whatever the integrator bumps when the credentials or the
 * expected peer name change. */
uint32_t FakeVersion_ReturnValue = 0U;
void* FakeVersion_LastContext = nullptr;

void FakeVersion_Reset()
{
    FakeVersion_ReturnValue = 0;
    FakeVersion_LastContext = reinterpret_cast<void*>(0x1U); /* sentinel - overwritten on first call */
}

extern "C" uint32_t FakeVersion(void* context)
{
    FakeVersion_LastContext = context;
    return FakeVersion_ReturnValue;
}

/* The TLS profile the stream pulls at Open. Tests set the fields they care
 * about; anything left alone is what an integrator would leave to the library. */
struct SolidSyslogWolfSslProfile FakeProfile_Value;
int FakeProfile_CallCount = 0;
void* FakeProfile_LastContext = nullptr;

void FakeProfile_Reset()
{
    FakeProfile_Value = {};
    FakeProfile_CallCount = 0;
    FakeProfile_LastContext = reinterpret_cast<void*>(0x1U); /* sentinel - overwritten on first call */
}

extern "C" void FakeProfile(struct SolidSyslogWolfSslProfile* profile, void* context)
{
    FakeProfile_CallCount++;
    FakeProfile_LastContext = context;
    *profile = FakeProfile_Value;
}

/* What the stream had already done when the credentials were asked to install. */
int NewsSeenAtInstall = -1;
int ConnectsSeenAtInstall = -1;

extern "C" void CaptureNewsAtInstall(void)
{
    NewsSeenAtInstall = WolfSslTlsFake_NewCallCount();
    ConnectsSeenAtInstall = WolfSslTlsFake_ConnectCallCount();
}

/* What wolfSSL had already let go of when the credentials were told the window
   had closed. */
int CtxFreesSeenAtRelease = -1;

extern "C" void CaptureCtxFreesAtRelease(void)
{
    CtxFreesSeenAtRelease = WolfSslTlsFake_CtxFreeCallCount();
}
} // namespace

// clang-format off
TEST_GROUP(SolidSyslogWolfSslStream)
{
    int                                   heap      = 0;
    struct SolidSyslogStream*             transport = nullptr;
    struct SolidSyslogStream*             handle    = nullptr;
    struct SolidSyslogWolfSslStreamConfig config    = {};
    struct SolidSyslogAddress*            addr      = nullptr;

    void setup() override
    {
        WolfSslTlsFake_Reset();
        WolfSslFake_Reset();
        WolfSslCredentialsFake_Reset();
        ErrorHandlerFake_Install(nullptr);
        FakeGetHandshakeTimeoutMs_Reset();
        FakeVersion_Reset();
        FakeProfile_Reset();
        FakeProfile_Value.ServerName = "syslog.example.com";
        NoOpSleepCallCount = 0;
        g_lastSleepMs = 0;
        NewsSeenAtInstall = -1;
        ConnectsSeenAtInstall = -1;
        CtxFreesSeenAtRelease = -1;
        transport = StreamFake_Create();
        config.Transport = transport;
        config.Sleep = NoOpSleep;
        config.Heap = &heap;
        config.Credentials = WolfSslCredentialsFake_Get();
        config.Profile = FakeProfile;
        handle = SolidSyslogWolfSslStream_Create(&config);
        addr = AddressFake_Get();
    }

    void teardown() override
    {
        SolidSyslogWolfSslStream_Destroy(handle);
        StreamFake_Destroy(transport);
    }

    /* Replaces the default Null-getter handle with one that uses the fake
     * handshake-timeout getter. Each test sets only the fake-getter return
     * value (or context) it needs different from the defaults restored in
     * setup(). */
    void RecreateHandleWithFakeHandshakeGetter()
    {
        SolidSyslogWolfSslStream_Destroy(handle);
        config.GetHandshakeTimeoutMs = FakeGetHandshakeTimeoutMs;
        handle                        = SolidSyslogWolfSslStream_Create(&config);
    }

    /* Arrange wolfSSL_connect to first want more I/O, then succeed on the next
     * call - exercises the bounded handshake retry loop's progress path. */
    static void ArrangeHandshakeRetryThenSucceed(int wantError)
    {
        int seq[] = {wantError, 0};
        WolfSslTlsFake_SetConnectErrorSequence(seq, 2);
    }

    /* Arrange wolfSSL_connect to fail with `errorCode` on every call - used
     * both for the persistent-WANT (budget-exhausted) and hard-error paths. */
    static void ArrangePersistentHandshakeError(int errorCode)
    {
        int seq[] = {errorCode};
        WolfSslTlsFake_SetConnectErrorSequence(seq, 1);
    }

    /* Arrange a peer the library refuses: the verify callback sees each of
     * `errors` in turn, then wolfSSL_connect fails with the last of them. */
    static void ArrangeCertificateVerificationFailure(const int* errors, int count)
    {
        WolfSslTlsFake_SetVerifyErrors(errors, count);
        ArrangePersistentHandshakeError(errors[count - 1]);
    }

    /* Arrange the call WOLFSSL_ALWAYS_VERIFY_CB makes for a leaf that verified,
     * which is where a pin is checked on a chain the library accepts. */
    static void ArrangeLeafThatVerifies()
    {
        int errors[] = {0};
        WolfSslTlsFake_SetVerifyErrors(errors, 1);
    }

    /* Drive the verify callback directly with `preverify` and a store carrying
     * `error`, and return what it answered. */
    [[nodiscard]] int OpenThenVerify(int preverify, int error) const
    {
        SolidSyslogStream_Open(handle, addr);
        auto* verify = WolfSslTlsFake_LastSetVerifyCallback();
        CHECK_TRUE_TEXT(verify != nullptr, "the stream registered no verify callback");
        int result = -1;
        if (verify != nullptr)
        {
            result = verify(preverify, WolfSslTlsFake_StoreWithError(error));
        }
        return result;
    }

    /* As OpenThenVerify, for the certificate at `depth` of the peer's chain. */
    [[nodiscard]] int OpenThenVerifyAt(int preverify, int error, int depth) const
    {
        SolidSyslogStream_Open(handle, addr);
        auto* verify = WolfSslTlsFake_LastSetVerifyCallback();
        CHECK_TRUE_TEXT(verify != nullptr, "the stream registered no verify callback");
        int result = -1;
        if (verify != nullptr)
        {
            result = verify(preverify, WolfSslTlsFake_StoreAt(error, depth));
        }
        return result;
    }

    /* Tests needing config tweaks call this to release setup()'s pool slot,
     * mutate `config`, then re-Create. Fully resets the fixture (transport,
     * fake counters, error handler) so the test body observes counts from
     * this Open onwards only. */
    void ReCreateHandleWithUpdatedConfig()
    {
        SolidSyslogWolfSslStream_Destroy(handle);
        StreamFake_Destroy(transport);
        WolfSslTlsFake_Reset();
        WolfSslCredentialsFake_Reset();
        ErrorHandlerFake_Install(nullptr);
        transport = StreamFake_Create();
        config.Transport = transport;
        handle = SolidSyslogWolfSslStream_Create(&config);
    }
};

// clang-format on

TEST(SolidSyslogWolfSslStream, OpenDelegatesToInjectedTransport)
{
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, StreamFake_OpenCallCount(transport));
    POINTERS_EQUAL(addr, StreamFake_LastOpenAddr(transport));
}

TEST(SolidSyslogWolfSslStream, OpenFailsWithoutTouchingTheLibraryWhenTheTransportCannotOpen)
{
    StreamFake_SetOpenFails(transport, true);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));
    LONGS_EQUAL(0, WolfSslTlsFake_ClientMethodExCallCount());
}

TEST(SolidSyslogWolfSslStream, OpenBuildsAClientContextOnTheConfiguredHeap)
{
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, WolfSslTlsFake_ClientMethodExCallCount());
    POINTERS_EQUAL(&heap, WolfSslTlsFake_LastClientMethodExHeap());
    LONGS_EQUAL(1, WolfSslTlsFake_CtxNewExCallCount());
    POINTERS_EQUAL(WolfSslTlsFake_Method(), WolfSslTlsFake_LastCtxNewExMethod());
    POINTERS_EQUAL(&heap, WolfSslTlsFake_LastCtxNewExHeap());
}

TEST(SolidSyslogWolfSslStream, OpenAsksTheCredentialsToInstallOntoItsContext)
{
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, WolfSslCredentialsFake_InstallCallCount());
    POINTERS_EQUAL(WolfSslTlsFake_Ctx(), WolfSslCredentialsFake_LastInstallContext());
}

TEST(SolidSyslogWolfSslStream, OpenCreatesTheSessionFromTheContextAfterTheCredentialsInstall)
{
    WolfSslCredentialsFake_SetInstallObserver(CaptureNewsAtInstall);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(0, NewsSeenAtInstall);
    LONGS_EQUAL(1, WolfSslTlsFake_NewCallCount());
    POINTERS_EQUAL(WolfSslTlsFake_Ctx(), WolfSslTlsFake_LastNewCtx());
}

TEST(SolidSyslogWolfSslStream, OpenPinsMinimumTlsVersionToTls12OnTheSession)
{
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, WolfSslTlsFake_SetMinVersionCallCount());
    POINTERS_EQUAL(WolfSslTlsFake_Session(), WolfSslTlsFake_LastSetMinVersionSsl());
    LONGS_EQUAL(3 /* WOLFSSL_TLSV1_2 */, WolfSslTlsFake_LastSetMinVersionArg());
}

TEST(SolidSyslogWolfSslStream, OpenRaisesTheRsaFloorTo2048Bits)
{
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(2048, WolfSslTlsFake_LastMinRsaKeySz());
}

TEST(SolidSyslogWolfSslStream, OpenRaisesTheEccFloorTo224Bits)
{
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(224, WolfSslTlsFake_LastMinEccKeySz());
}

TEST(SolidSyslogWolfSslStream, OpenRaisesTheDiffieHellmanFloorTo2048Bits)
{
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(2048, WolfSslTlsFake_LastMinDhKeySz());
}

TEST(SolidSyslogWolfSslStream, OpenAppliesTheProfilesCipherPolicyToTheSession)
{
    FakeProfile_Value.CipherList = "ECDHE-RSA-AES128-GCM-SHA256";

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, WolfSslTlsFake_SetCipherListCallCount());
    POINTERS_EQUAL(WolfSslTlsFake_Session(), WolfSslTlsFake_LastSetCipherListSsl());
    STRCMP_EQUAL("ECDHE-RSA-AES128-GCM-SHA256", WolfSslTlsFake_LastSetCipherList());
}

TEST(SolidSyslogWolfSslStream, OpenLeavesTheBuildsCiphersuitesAloneWhenTheProfileNamesNoPolicy)
{
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(0, WolfSslTlsFake_SetCipherListCallCount());
}

TEST(SolidSyslogWolfSslStream, OpenSetsTheExpectedPeerNameOnTheSession)
{
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, WolfSslTlsFake_CheckDomainNameCallCount());
    POINTERS_EQUAL(WolfSslTlsFake_Session(), WolfSslTlsFake_LastCheckDomainNameSsl());
    STRCMP_EQUAL("syslog.example.com", WolfSslTlsFake_LastCheckDomainName());
}

TEST(SolidSyslogWolfSslStream, OpenSendsTheExpectedPeerNameAsTheServerNameIndication)
{
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, WolfSslTlsFake_UseSniCallCount());
    POINTERS_EQUAL(WolfSslTlsFake_Session(), WolfSslTlsFake_LastUseSniSsl());
    LONGS_EQUAL(0 /* WOLFSSL_SNI_HOST_NAME */, WolfSslTlsFake_LastUseSniType());
    STRCMP_EQUAL("syslog.example.com", static_cast<const char*>(WolfSslTlsFake_LastUseSniData()));
    LONGS_EQUAL(strlen("syslog.example.com"), WolfSslTlsFake_LastUseSniSize());
}

TEST(SolidSyslogWolfSslStream, OpenDeclaresAnAddressLiteralAsTheExpectedAddressRatherThanAName)
{
    FakeProfile_Value.ServerName = "127.0.0.1";

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(0, WolfSslTlsFake_CheckDomainNameCallCount());
    LONGS_EQUAL(1, WolfSslTlsFake_CheckIpAddressCallCount());
    POINTERS_EQUAL(WolfSslTlsFake_Session(), WolfSslTlsFake_LastCheckIpAddressSsl());
    STRCMP_EQUAL("127.0.0.1", WolfSslTlsFake_LastCheckIpAddress());
}

TEST(SolidSyslogWolfSslStream, OpenDeclaresAnIpv6LiteralAsTheExpectedAddress)
{
    FakeProfile_Value.ServerName = "fe80::1";

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(0, WolfSslTlsFake_CheckDomainNameCallCount());
    STRCMP_EQUAL("fe80::1", WolfSslTlsFake_LastCheckIpAddress());
}

TEST(SolidSyslogWolfSslStream, OpenSendsAnAddressLiteralAsTheServerNameIndicationToo)
{
    FakeProfile_Value.ServerName = "127.0.0.1";

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, WolfSslTlsFake_UseSniCallCount());
    STRCMP_EQUAL("127.0.0.1", static_cast<const char*>(WolfSslTlsFake_LastUseSniData()));
}

TEST(SolidSyslogWolfSslStream, OpenDeclaresANameWithDigitsInItAsAName)
{
    FakeProfile_Value.ServerName = "collector1.example.com";

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, WolfSslTlsFake_CheckDomainNameCallCount());
    LONGS_EQUAL(0, WolfSslTlsFake_CheckIpAddressCallCount());
}

TEST(SolidSyslogWolfSslStream, OpenReportsAnExpectedAddressTheLibraryWouldNotTake)
{
    FakeProfile_Value.ServerName = "127.0.0.1";
    WolfSslTlsFake_SetCheckIpAddressReturn(FAKE_WOLFSSL_FAILURE);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(0, WolfSslTlsFake_ConnectCallCount());
    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_STREAM_ERROR_SERVER_NAME_NOT_APPLIED
    );
}

TEST(SolidSyslogWolfSslStream, OpenInstallsTheIoCallbacksWithTheStreamAsTheirContext)
{
    SolidSyslogStream_Open(handle, addr);

    CHECK_TRUE(WolfSslTlsFake_LastIoRecvCallback() != nullptr);
    CHECK_TRUE(WolfSslTlsFake_LastIoSendCallback() != nullptr);
    POINTERS_EQUAL(handle, WolfSslTlsFake_LastIoReadCtx());
    POINTERS_EQUAL(handle, WolfSslTlsFake_LastIoWriteCtx());
}

TEST(SolidSyslogWolfSslStream, OpenRequiresAPeerCertificateAndWatchesItsVerification)
{
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, WolfSslTlsFake_SetVerifyCallCount());
    LONGS_EQUAL(3 /* WOLFSSL_VERIFY_PEER | WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT */, WolfSslTlsFake_LastSetVerifyMode());
    CHECK_TRUE(WolfSslTlsFake_LastSetVerifyCallback() != nullptr);
    POINTERS_EQUAL(handle, WolfSslTlsFake_LastCertCbCtx());
}

TEST(SolidSyslogWolfSslStream, OpenDrivesTheHandshakeOnTheSession)
{
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, WolfSslTlsFake_ConnectCallCount());
    POINTERS_EQUAL(WolfSslTlsFake_Session(), WolfSslTlsFake_LastConnectArg());
}

TEST(SolidSyslogWolfSslStream, OpenReturnsTrueWhenTheHandshakeSucceeds)
{
    CHECK_TRUE(SolidSyslogStream_Open(handle, addr));
}

TEST(SolidSyslogWolfSslStream, OpenPullsTheProfileWithItsContext)
{
    config.ProfileContext = &heap;
    ReCreateHandleWithUpdatedConfig();

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, FakeProfile_CallCount);
    POINTERS_EQUAL(&heap, FakeProfile_LastContext);
}

TEST(SolidSyslogWolfSslStream, CloseDelegatesToInjectedTransport)
{
    SolidSyslogStream_Open(handle, addr);

    SolidSyslogStream_Close(handle);

    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
}

TEST(SolidSyslogWolfSslStream, VersionReportsTheConfiguredFunctionsValue)
{
    config.Version = FakeVersion;
    ReCreateHandleWithUpdatedConfig();
    FakeVersion_ReturnValue = 7U;

    UNSIGNED_LONGS_EQUAL(7U, SolidSyslogStream_Version(handle));
}

TEST(SolidSyslogWolfSslStream, VersionFunctionReceivesVersionContext)
{
    config.Version = FakeVersion;
    config.VersionContext = &heap;
    ReCreateHandleWithUpdatedConfig();

    SolidSyslogStream_Version(handle);

    POINTERS_EQUAL(&heap, FakeVersion_LastContext);
}

TEST(SolidSyslogWolfSslStream, VersionIsZeroWhenNoFunctionIsConfigured)
{
    UNSIGNED_LONGS_EQUAL(0U, SolidSyslogStream_Version(handle));
}

TEST(SolidSyslogWolfSslStream, TheErrorSourceNamesItsDetailsThroughTlsStreamErrors)
{
    POINTERS_EQUAL(SolidSyslogTlsStreamErrors_Name, SolidSyslogWolfSslStreamErrorSource.DetailName);
}

TEST(SolidSyslogWolfSslStream, OpenReportsThatTheContextCouldNotBeBuilt)
{
    WolfSslTlsFake_SetCtxNewExReturnsNull(true);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(0, WolfSslCredentialsFake_InstallCallCount());
    LONGS_EQUAL(0, WolfSslTlsFake_CtxFreeCallCount());
    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &SolidSyslogWolfSslStreamErrorSource,
        SOLIDSYSLOG_CAT_TLS_STREAM_INIT_FAILED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_CONTEXT_INIT_FAILED
    );
}

TEST(SolidSyslogWolfSslStream, OpenFailsWhenTheCredentialsCannotInstall)
{
    WolfSslCredentialsFake_SetInstallSucceeds(false);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(0, WolfSslTlsFake_NewCallCount());
    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
    LONGS_EQUAL(1, WolfSslTlsFake_CtxFreeCallCount());
    LONGS_EQUAL(1, WolfSslCredentialsFake_ReleaseCallCount());
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogWolfSslStream, OpenFailsWhenNothingAuthorisesThePeer)
{
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(0, WolfSslTlsFake_NewCallCount());
    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_STREAM_ERROR_NO_PEER_AUTHORISATION
    );
}

TEST(SolidSyslogWolfSslStream, OpenWithPinsAndNoAnchorsGoesOnToTheHandshake)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, WolfSslTlsFake_ConnectCallCount());
}

TEST(SolidSyslogWolfSslStream, OpenRefusesAPinThatIsNotWellFormed)
{
    const char* const pins[] = {"sha-256:not-a-digest"};
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(0, WolfSslTlsFake_NewCallCount());
    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_STREAM_ERROR_FINGERPRINT_MALFORMED
    );
}

TEST(SolidSyslogWolfSslStream, OpenWarnsOfASha1PinAndStillConnects)
{
    const char* const pins[] = {TEST_SHA1_PIN};
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);
    ArrangeLeafThatVerifies();

    CHECK_TRUE(SolidSyslogStream_Open(handle, addr));

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogWolfSslStreamErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_STREAM_ERROR_FINGERPRINT_SHA1
    );
}

TEST(SolidSyslogWolfSslStream, OpenReportsThatTheSessionCouldNotBeCreated)
{
    WolfSslTlsFake_SetNewReturnsNull(true);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(0, WolfSslTlsFake_FreeCallCount());
    LONGS_EQUAL(1, WolfSslTlsFake_CtxFreeCallCount());
    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
    LONGS_EQUAL(1, WolfSslCredentialsFake_ReleaseCallCount());
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &SolidSyslogWolfSslStreamErrorSource,
        SOLIDSYSLOG_CAT_TLS_STREAM_INIT_FAILED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_SESSION_INIT_FAILED
    );
}

TEST(SolidSyslogWolfSslStream, OpenReportsATlsFloorTheLibraryWouldNotApply)
{
    WolfSslTlsFake_SetSetMinVersionReturn(FAKE_WOLFSSL_FAILURE);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(0, WolfSslTlsFake_ConnectCallCount());
    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_TLS_STREAM_INIT_FAILED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_DEFAULTS_NOT_APPLIED
    );
}

TEST(SolidSyslogWolfSslStream, OpenReportsAnRsaFloorTheLibraryWouldNotApply)
{
    WolfSslTlsFake_SetSetMinRsaKeySzReturn(FAKE_WOLFSSL_FAILURE);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_TLS_STREAM_INIT_FAILED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_DEFAULTS_NOT_APPLIED
    );
}

TEST(SolidSyslogWolfSslStream, OpenReportsAnEccFloorTheLibraryWouldNotApply)
{
    WolfSslTlsFake_SetSetMinEccKeySzReturn(FAKE_WOLFSSL_FAILURE);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_TLS_STREAM_INIT_FAILED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_DEFAULTS_NOT_APPLIED
    );
}

TEST(SolidSyslogWolfSslStream, OpenReportsADiffieHellmanFloorTheLibraryWouldNotApply)
{
    WolfSslTlsFake_SetSetMinDhKeySzReturn(FAKE_WOLFSSL_FAILURE);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_TLS_STREAM_INIT_FAILED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_DEFAULTS_NOT_APPLIED
    );
}

TEST(SolidSyslogWolfSslStream, OpenReportsACipherPolicyTheLibraryRejects)
{
    FakeProfile_Value.CipherList = "NOT-A-SUITE-THIS-BUILD-KNOWS";
    WolfSslTlsFake_SetSetCipherListReturn(FAKE_WOLFSSL_FAILURE);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(0, WolfSslTlsFake_ConnectCallCount());
    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_STREAM_ERROR_CIPHER_POLICY_REJECTED
    );
}

TEST(SolidSyslogWolfSslStream, OpenReportsAPeerNameTheLibraryWouldNotTake)
{
    WolfSslTlsFake_SetCheckDomainNameReturn(FAKE_WOLFSSL_FAILURE);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(0, WolfSslTlsFake_ConnectCallCount());
    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_STREAM_ERROR_SERVER_NAME_NOT_APPLIED
    );
}

TEST(SolidSyslogWolfSslStream, OpenReportsAServerNameIndicationTheLibraryWouldNotTake)
{
    WolfSslTlsFake_SetUseSniReturn(FAKE_WOLFSSL_FAILURE);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_STREAM_ERROR_SERVER_NAME_NOT_APPLIED
    );
}

TEST(SolidSyslogWolfSslStream, OpenReportsTheLibraryOutOfMemoryWhenTheServerNameIndicationCannotAllocate)
{
    WolfSslTlsFake_SetUseSniReturn(FAKE_MEMORY_E);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_STREAM_ERROR_LIBRARY_OUT_OF_MEMORY
    );
}

TEST(SolidSyslogWolfSslStream, OpenWarnsWhenServerNameIsNull)
{
    FakeProfile_Value.ServerName = nullptr;

    SolidSyslogStream_Open(handle, addr);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogWolfSslStreamErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_STREAM_ERROR_SERVER_NAME_NOT_SET
    );
}

TEST(SolidSyslogWolfSslStream, OpenDoesNotWarnOfAnUnnamedPeerThatAPinNames)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);
    ArrangeLeafThatVerifies();
    FakeProfile_Value.ServerName = nullptr;

    CHECK_TRUE(SolidSyslogStream_Open(handle, addr));

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogWolfSslStream, OpenNamesNoPeerAndStillConnectsWhenServerNameIsNull)
{
    FakeProfile_Value.ServerName = nullptr;

    CHECK_TRUE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(0, WolfSslTlsFake_CheckDomainNameCallCount());
    LONGS_EQUAL(0, WolfSslTlsFake_UseSniCallCount());
    LONGS_EQUAL(1, WolfSslTlsFake_ConnectCallCount());
}

TEST(SolidSyslogWolfSslStream, OpenDoesNotWarnWhenServerNameIsEmpty)
{
    FakeProfile_Value.ServerName = "";

    SolidSyslogStream_Open(handle, addr);

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogWolfSslStream, OpenNamesNoPeerAndStillConnectsWhenServerNameIsEmpty)
{
    FakeProfile_Value.ServerName = "";

    CHECK_TRUE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(0, WolfSslTlsFake_CheckDomainNameCallCount());
    LONGS_EQUAL(0, WolfSslTlsFake_UseSniCallCount());
    LONGS_EQUAL(1, WolfSslTlsFake_ConnectCallCount());
}

TEST(SolidSyslogWolfSslStream, OpeningAStreamThatIsAlreadyOpenIsRefusedAndReported)
{
    SolidSyslogStream_Open(handle, addr);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_BAD_ARGUMENT_SEVERITY,
        &SolidSyslogWolfSslStreamErrorSource,
        SOLIDSYSLOG_CAT_BAD_ARGUMENT,
        SOLIDSYSLOG_TLS_STREAM_ERROR_ALREADY_OPEN
    );
}

TEST(SolidSyslogWolfSslStream, OpeningAStreamThatIsAlreadyOpenLeavesTheSessionItHoldsUntouched)
{
    SolidSyslogStream_Open(handle, addr);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, StreamFake_OpenCallCount(transport));
    LONGS_EQUAL(1, WolfSslTlsFake_NewCallCount());
    LONGS_EQUAL(0, WolfSslTlsFake_FreeCallCount());
    LONGS_EQUAL(0, StreamFake_CloseCallCount(transport));
}

/* -------------------------------------------------------------------------
 * Bounded handshake retry loop. wolfSSL_connect under non-blocking transport
 * answers WANT_READ / WANT_WRITE between RTTs; the loop must drive it to
 * completion within a bounded budget so a wedged peer doesn't burn the service
 * thread indefinitely.
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogWolfSslStream, OpenReturnsFalseWhenTheHandshakeFails)
{
    ArrangePersistentHandshakeError(FAKE_FATAL_ERROR);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));
}

TEST(SolidSyslogWolfSslStream, OpenRetriesTheHandshakeOnWantRead)
{
    ArrangeHandshakeRetryThenSucceed(FAKE_WOLFSSL_ERROR_WANT_READ);

    CHECK_TRUE(SolidSyslogStream_Open(handle, addr));
    CALLED_FAKE(WolfSslTlsFake_Connect, TWICE);
}

TEST(SolidSyslogWolfSslStream, OpenSleepsBetweenHandshakeRetries)
{
    ArrangeHandshakeRetryThenSucceed(FAKE_WOLFSSL_ERROR_WANT_READ);

    SolidSyslogStream_Open(handle, addr);

    CALLED_FUNCTION(NoOpSleep, ONCE);
}

TEST(SolidSyslogWolfSslStream, OpenRetriesTheHandshakeOnWantWrite)
{
    ArrangeHandshakeRetryThenSucceed(FAKE_WOLFSSL_ERROR_WANT_WRITE);

    CHECK_TRUE(SolidSyslogStream_Open(handle, addr));
    CALLED_FAKE(WolfSslTlsFake_Connect, TWICE);
}

TEST(SolidSyslogWolfSslStream, OpenClosesTransportAndFreesTheSessionWhenTheHandshakeBudgetExhausts)
{
    ArrangePersistentHandshakeError(FAKE_WOLFSSL_ERROR_WANT_READ);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CHECK_OPEN_UNWOUND_WITH_SEVERITY(
        transport,
        SOLIDSYSLOG_SEVERITY_WARNING,
        SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_HANDSHAKE_TIMEOUT
    );
}

TEST(SolidSyslogWolfSslStream, OpenInvokesTheConfiguredHandshakeTimeoutGetter)
{
    RecreateHandleWithFakeHandshakeGetter();

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, FakeGetHandshakeTimeoutMs_CallCount);
}

TEST(SolidSyslogWolfSslStream, OpenUsesTheGettersReturnValueAsTheHandshakeBudget)
{
    /* 50 ms budget against the 10 ms poll interval -> five sleeps before
       declaring HANDSHAKE_TIMEOUT and unwinding. */
    FakeGetHandshakeTimeoutMs_ReturnValue = 50U;
    RecreateHandleWithFakeHandshakeGetter();
    ArrangePersistentHandshakeError(FAKE_WOLFSSL_ERROR_WANT_READ);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(5, NoOpSleepCallCount);
}

TEST(SolidSyslogWolfSslStream, HandshakePollSleepsTenMilliseconds)
{
    ArrangeHandshakeRetryThenSucceed(FAKE_WOLFSSL_ERROR_WANT_READ);

    SolidSyslogStream_Open(handle, addr);

    UNSIGNED_LONGS_EQUAL(10U, g_lastSleepMs);
}

TEST(SolidSyslogWolfSslStream, HandshakeLastPollSleepsOnlyWhatIsLeftOfTheBudget)
{
    FakeGetHandshakeTimeoutMs_ReturnValue = 25U;
    RecreateHandleWithFakeHandshakeGetter();
    ArrangePersistentHandshakeError(FAKE_WOLFSSL_ERROR_WANT_READ);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(3, NoOpSleepCallCount);
    UNSIGNED_LONGS_EQUAL(5U, g_lastSleepMs);
}

TEST(SolidSyslogWolfSslStream, TheGetterReceivesANullContextWhenNoneIsConfigured)
{
    RecreateHandleWithFakeHandshakeGetter();

    SolidSyslogStream_Open(handle, addr);

    POINTERS_EQUAL(nullptr, FakeGetHandshakeTimeoutMs_LastContext);
}

TEST(SolidSyslogWolfSslStream, TheGetterReceivesTheConfiguredContext)
{
    config.HandshakeTimeoutContext = &heap;
    RecreateHandleWithFakeHandshakeGetter();

    SolidSyslogStream_Open(handle, addr);

    POINTERS_EQUAL(&heap, FakeGetHandshakeTimeoutMs_LastContext);
}

TEST(SolidSyslogWolfSslStream, ASecondOpenAfterAFailedFirstOpenSucceeds)
{
    int handshakeSequence[] = {FAKE_FATAL_ERROR, 0};
    WolfSslTlsFake_SetConnectErrorSequence(handshakeSequence, 2);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));
    CHECK_TRUE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(2, StreamFake_OpenCallCount(transport));
    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
    LONGS_EQUAL(2, WolfSslTlsFake_CtxNewExCallCount());
    LONGS_EQUAL(2, WolfSslTlsFake_NewCallCount());
}

TEST(SolidSyslogWolfSslStream, OpenClosesTransportAndFreesTheSessionWhenTheHandshakeFailsHard)
{
    ArrangePersistentHandshakeError(FAKE_FATAL_ERROR);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CALLED_FAKE(WolfSslTlsFake_Connect, ONCE);
    CALLED_FUNCTION(NoOpSleep, NEVER);
    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_HANDSHAKE_REJECTED
    );
}

TEST(SolidSyslogWolfSslStream, OpenReportsTheLibraryOutOfMemoryWhenTheHandshakeCannotAllocate)
{
    ArrangePersistentHandshakeError(FAKE_MEMORY_E);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_LIBRARY_OUT_OF_MEMORY
    );
}

/* The sender retries a refused handshake every service pass; one run of the
 * same refusal is one report. */
TEST(SolidSyslogWolfSslStream, OpenReportsARepeatedHandshakeRejectionOnce)
{
    ArrangePersistentHandshakeError(FAKE_FATAL_ERROR);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));
    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
}

TEST(SolidSyslogWolfSslStream, OpenReportsADifferentHandshakeFailureInTheSameRun)
{
    ArrangePersistentHandshakeError(FAKE_FATAL_ERROR);
    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));
    ArrangePersistentHandshakeError(FAKE_WOLFSSL_ERROR_WANT_READ);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_HANDSHAKE_TIMEOUT, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWolfSslStream, OpenReportsTheSameHandshakeRejectionAgainAfterASessionInBetween)
{
    ArrangePersistentHandshakeError(FAKE_FATAL_ERROR);
    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));
    ArrangePersistentHandshakeError(0);
    CHECK_TRUE(SolidSyslogStream_Open(handle, addr));
    SolidSyslogStream_Close(handle);
    ArrangePersistentHandshakeError(FAKE_FATAL_ERROR);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
}

/* -------------------------------------------------------------------------
 * The refusal names the check the peer failed. wolfSSL tells the verify
 * callback one error per call and may call more than once; the stream records
 * them all and names the first in the contract's order.
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogWolfSslStream, OpenReportsThatThePeerCertificateHasExpired)
{
    int errors[] = {FAKE_ASN_AFTER_DATE_E};
    ArrangeCertificateVerificationFailure(errors, 1);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_EXPIRED
    );
}

TEST(SolidSyslogWolfSslStream, OpenReportsThatThePeerCertificateIsNotYetValid)
{
    int errors[] = {FAKE_ASN_BEFORE_DATE_E};
    ArrangeCertificateVerificationFailure(errors, 1);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_NOT_YET_VALID
    );
}

TEST(SolidSyslogWolfSslStream, OpenReportsThatThePeerNameDidNotMatch)
{
    int errors[] = {FAKE_DOMAIN_NAME_MISMATCH};
    ArrangeCertificateVerificationFailure(errors, 1);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_NAME_MISMATCHED
    );
}

TEST(SolidSyslogWolfSslStream, OpenReportsThatThePeerAddressDidNotMatchAsANameMismatch)
{
    int errors[] = {FAKE_IPADDR_MISMATCH};
    ArrangeCertificateVerificationFailure(errors, 1);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_NAME_MISMATCHED
    );
}

TEST(SolidSyslogWolfSslStream, OpenReportsThatThePeerCertificateIsNotTrusted)
{
    int errors[] = {FAKE_ASN_NO_SIGNER_E};
    ArrangeCertificateVerificationFailure(errors, 1);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_UNTRUSTED
    );
}

TEST(SolidSyslogWolfSslStream, OpenReportsAVerificationFailureItCannotNameAsUntrusted)
{
    int errors[] = {FAKE_ASN_SIG_CONFIRM_E};
    ArrangeCertificateVerificationFailure(errors, 1);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_UNTRUSTED
    );
}

TEST(SolidSyslogWolfSslStream, OpenReportsAnUntrustedChainAheadOfTheDatesOnIt)
{
    int errors[] = {FAKE_ASN_AFTER_DATE_E, FAKE_ASN_NO_SIGNER_E};
    ArrangeCertificateVerificationFailure(errors, 2);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_UNTRUSTED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWolfSslStream, OpenReportsAnUntrustedChainAheadOfTheNameOnItWhicheverWasRaisedLast)
{
    int errors[] = {FAKE_ASN_NO_SIGNER_E, FAKE_DOMAIN_NAME_MISMATCH};
    ArrangeCertificateVerificationFailure(errors, 2);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_UNTRUSTED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWolfSslStream, OpenReportsANameMismatchAheadOfTheDatesWhicheverWasRaisedLast)
{
    int errors[] = {FAKE_DOMAIN_NAME_MISMATCH, FAKE_ASN_AFTER_DATE_E};
    ArrangeCertificateVerificationFailure(errors, 2);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_NAME_MISMATCHED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWolfSslStream, OpenReportsAnExpiredCertificateAheadOfOneNotYetValid)
{
    int errors[] = {FAKE_ASN_BEFORE_DATE_E, FAKE_ASN_AFTER_DATE_E};
    ArrangeCertificateVerificationFailure(errors, 2);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_EXPIRED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWolfSslStream, OpenReportsAPeerWhoseCertificateMatchesNoPin)
{
    const char* const pins[] = {TEST_UNMATCHED_SHA256_PIN};
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);
    int errors[] = {FAKE_ASN_NO_SIGNER_E};
    ArrangeCertificateVerificationFailure(errors, 1);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_FINGERPRINT_MISMATCHED
    );
}

TEST(SolidSyslogWolfSslStream, OpenReportsAPinWhoseDigestTheBuildCannotCompute)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);
    WolfSslFake_SetHashUnavailable(SOLIDSYSLOG_TLS_HASH_SHA256);
    int errors[] = {FAKE_ASN_NO_SIGNER_E};
    ArrangeCertificateVerificationFailure(errors, 1);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_FINGERPRINT_DIGEST_UNAVAILABLE, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWolfSslStream, OpenReportsAPinnedPeerWhoseCertificateHasExpired)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);
    WolfSslFake_SetParseCertReturn(FAKE_ASN_AFTER_DATE_E);
    int errors[] = {FAKE_ASN_NO_SIGNER_E};
    ArrangeCertificateVerificationFailure(errors, 1);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_EXPIRED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWolfSslStream, OpenReportsAPinnedPeerWhoseNameDoesNotMatch)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);
    int errors[] = {FAKE_ASN_NO_SIGNER_E, FAKE_DOMAIN_NAME_MISMATCH};
    ArrangeCertificateVerificationFailure(errors, 2);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(0, WolfSslTlsFake_LastVerifyCallbackResult());
    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_NAME_MISMATCHED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWolfSslStream, OpenReportsAPinMatchingNothingAheadOfEveryObjectionToTheChain)
{
    const char* const pins[] = {TEST_UNMATCHED_SHA256_PIN};
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);
    int errors[] = {FAKE_ASN_AFTER_DATE_E, FAKE_DOMAIN_NAME_MISMATCH, FAKE_ASN_NO_SIGNER_E};
    ArrangeCertificateVerificationFailure(errors, 3);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_FINGERPRINT_MISMATCHED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWolfSslStream, OpenRefusesAPinnedPeerWhoseHandshakeCompletedWithoutAPinMatching)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CHECK_OPEN_UNWOUND_WITH_ERROR(
        transport,
        SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_FINGERPRINT_MISMATCHED
    );
}

TEST(SolidSyslogWolfSslStream, OpenConnectsAPinnedPeerWithNoAnchorsOnceItsMissingSignerIsWaived)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);
    int errors[] = {FAKE_ASN_NO_SIGNER_E};
    WolfSslTlsFake_SetVerifyErrors(errors, 1);

    CHECK_TRUE(SolidSyslogStream_Open(handle, addr));

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogWolfSslStream, APinMatchedOnOneOpenDoesNotConfirmTheNext)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);
    ArrangeLeafThatVerifies();
    CHECK_TRUE(SolidSyslogStream_Open(handle, addr));
    SolidSyslogStream_Close(handle);
    WolfSslTlsFake_SetVerifyErrors(nullptr, 0);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_FINGERPRINT_MISMATCHED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWolfSslStream, OpenReportsThePeersRefusalWhenTheHandshakeEndsOnTheLibrarysOwnVerdictCode)
{
    int errors[] = {FAKE_ASN_NO_SIGNER_E};
    WolfSslTlsFake_SetVerifyErrors(errors, 1);
    ArrangePersistentHandshakeError(FAKE_VERIFY_CERT_ERROR);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_UNTRUSTED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWolfSslStream, ARefusalRecordedOnOneOpenDoesNotNameTheNext)
{
    int errors[] = {FAKE_ASN_NO_SIGNER_E};
    ArrangeCertificateVerificationFailure(errors, 1);
    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));
    WolfSslTlsFake_SetVerifyErrors(nullptr, 0);
    ArrangePersistentHandshakeError(FAKE_FATAL_ERROR);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_HANDSHAKE_REJECTED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWolfSslStream, VerifyCallbackWaivesTheMissingSignerOfAPinnedPeerWithNoAnchors)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    LONGS_EQUAL(1, OpenThenVerify(0, FAKE_ASN_NO_SIGNER_E));
}

TEST(SolidSyslogWolfSslStream, VerifyCallbackRefusesAPeerWhoseCertificateMatchesNoPin)
{
    const char* const pins[] = {TEST_UNMATCHED_SHA256_PIN};
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    LONGS_EQUAL(0, OpenThenVerify(0, FAKE_ASN_NO_SIGNER_E));
}

TEST(SolidSyslogWolfSslStream, VerifyCallbackAuthorisesASha1PinByTheSha1DigestOfTheLeaf)
{
    const char* const pins[] = {TEST_SHA1_PIN};
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    LONGS_EQUAL(1, OpenThenVerify(0, FAKE_ASN_NO_SIGNER_E));
}

TEST(SolidSyslogWolfSslStream, VerifyCallbackRefusesAPinnedPeerWhoseDatesTheLibraryDidNotReach)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);
    WolfSslFake_SetParseCertReturn(FAKE_ASN_AFTER_DATE_E);

    LONGS_EQUAL(0, OpenThenVerify(0, FAKE_ASN_NO_SIGNER_E));
}

TEST(SolidSyslogWolfSslStream, VerifyCallbackAuthorisesByTheLeafWhicheverCertificateItIsCalledFor)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    (void) OpenThenVerifyAt(0, FAKE_ASN_NO_SIGNER_E, 1);

    POINTERS_EQUAL(WolfSslTlsFake_LeafDer(), WolfSslFake_LastHashData());
    UNSIGNED_LONGS_EQUAL(WolfSslTlsFake_LeafDerLength(), WolfSslFake_LastHashDataLen());
}

TEST(SolidSyslogWolfSslStream, VerifyCallbackChecksTheCertificateItIsCalledForAsATrustAnchorOnTheConfiguredHeap)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    (void) OpenThenVerifyAt(0, FAKE_ASN_NO_SIGNER_E, 1);

    POINTERS_EQUAL(WolfSslTlsFake_IssuerDer(), WolfSslFake_LastDecodedCertSource());
    UNSIGNED_LONGS_EQUAL(WolfSslTlsFake_IssuerDerLength(), WolfSslFake_LastDecodedCertSourceLen());
    POINTERS_EQUAL(&heap, WolfSslFake_LastDecodedCertHeap());
    LONGS_EQUAL(FAKE_CA_TYPE, WolfSslFake_LastParseCertType());
    LONGS_EQUAL(FAKE_VERIFY, WolfSslFake_LastParseCertVerify());
    POINTERS_EQUAL(nullptr, WolfSslFake_LastParseCertCm());
}

TEST(SolidSyslogWolfSslStream, VerifyCallbackFreesTheCertificateItChecked)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    (void) OpenThenVerify(0, FAKE_ASN_NO_SIGNER_E);

    LONGS_EQUAL(1, WolfSslFake_FreeDecodedCertCallCount());
    LONGS_EQUAL(0, WolfSslFake_DecodedCertsOutstanding());
}

TEST(SolidSyslogWolfSslStream, VerifyCallbackRefusesAPeerWhoseChainVerifiesButWhoseLeafMatchesNoPin)
{
    const char* const pins[] = {TEST_UNMATCHED_SHA256_PIN};
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    LONGS_EQUAL(0, OpenThenVerify(1, 0));
}

TEST(SolidSyslogWolfSslStream, VerifyCallbackWaivesNoMissingSignerWhereAnchorsAreInstalled)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    LONGS_EQUAL(0, OpenThenVerify(0, FAKE_ASN_NO_SIGNER_E));
}

TEST(SolidSyslogWolfSslStream, VerifyCallbackWaivesNoObjectionToAPinnedPeerButTheMissingSigner)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    LONGS_EQUAL(0, OpenThenVerify(0, FAKE_ASN_SIG_CONFIRM_E));
}

TEST(SolidSyslogWolfSslStream, VerifyCallbackLeavesAPinnedPeerWithAnchorsToTheLibrarysVerdict)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    LONGS_EQUAL(1, OpenThenVerify(1, 0));
}

TEST(SolidSyslogWolfSslStream, VerifyCallbackLeavesEnforcementToTheLibrary)
{
    LONGS_EQUAL(0, OpenThenVerify(0, FAKE_ASN_NO_SIGNER_E));
}

TEST(SolidSyslogWolfSslStream, VerifyCallbackAcceptsWhatTheLibraryAccepted)
{
    LONGS_EQUAL(1, OpenThenVerify(1, 0));
}

/* -------------------------------------------------------------------------
 * Send: reads what the peer has sent first, then writes all or nothing.
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogWolfSslStream, SendForwardsBufferToSslWriteOnTheSession)
{
    const unsigned char payload[] = {0x10, 0x20, 0x30};
    SolidSyslogStream_Open(handle, addr);

    SolidSyslogStream_Send(handle, payload, sizeof(payload));

    LONGS_EQUAL(1, WolfSslTlsFake_WriteCallCount());
    POINTERS_EQUAL(WolfSslTlsFake_Session(), WolfSslTlsFake_LastWriteSsl());
    POINTERS_EQUAL(payload, WolfSslTlsFake_LastWriteData());
    LONGS_EQUAL(sizeof(payload), WolfSslTlsFake_LastWriteSz());
}

TEST(SolidSyslogWolfSslStream, SendReturnsTrueWhenSslWriteWritesAllBytes)
{
    const unsigned char payload[] = {0x10, 0x20, 0x30};
    SolidSyslogStream_Open(handle, addr);

    CHECK_TRUE(SolidSyslogStream_Send(handle, payload, sizeof(payload)));
}

TEST(SolidSyslogWolfSslStream, SendReturnsFalseWhenSslWriteWritesPartial)
{
    const unsigned char payload[] = {0x10, 0x20, 0x30};
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetWriteReturn(2);

    CHECK_FALSE(SolidSyslogStream_Send(handle, payload, sizeof(payload)));
}

TEST(SolidSyslogWolfSslStream, SendReturnsFalseWhenSslWriteFails)
{
    const unsigned char payload[] = {0x10, 0x20, 0x30};
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetWriteReturn(-1);

    CHECK_FALSE(SolidSyslogStream_Send(handle, payload, sizeof(payload)));
}

TEST(SolidSyslogWolfSslStream, SendClosesTheSessionAndTransportOnWriteFailure)
{
    const unsigned char payload[] = {0x10, 0x20, 0x30};
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetWriteReturn(-1);

    SolidSyslogStream_Send(handle, payload, sizeof(payload));

    LONGS_EQUAL(1, WolfSslTlsFake_FreeCallCount());
    LONGS_EQUAL(1, WolfSslTlsFake_CtxFreeCallCount());
    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
}

TEST(SolidSyslogWolfSslStream, SendClosesTheSessionAndTransportOnShortWrite)
{
    const unsigned char payload[] = {0x10, 0x20, 0x30};
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetWriteReturn(2);

    SolidSyslogStream_Send(handle, payload, sizeof(payload));

    LONGS_EQUAL(1, WolfSslTlsFake_FreeCallCount());
    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
}

TEST(SolidSyslogWolfSslStream, SendRefusesTheRecordWhenThePeerHasClosedTheSession)
{
    const unsigned char payload[] = {0x10, 0x20, 0x30};
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetReadReturn(0, FAKE_WOLFSSL_ERROR_ZERO_RETURN);

    CHECK_FALSE(SolidSyslogStream_Send(handle, payload, sizeof(payload)));
}

TEST(SolidSyslogWolfSslStream, SendWritesNothingToASessionThePeerHasClosed)
{
    const unsigned char payload[] = {0x10, 0x20, 0x30};
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetReadReturn(0, FAKE_WOLFSSL_ERROR_ZERO_RETURN);

    SolidSyslogStream_Send(handle, payload, sizeof(payload));

    LONGS_EQUAL(0, WolfSslTlsFake_WriteCallCount());
}

TEST(SolidSyslogWolfSslStream, SendClosesTheSessionAndTransportWhenThePeerHasClosedTheSession)
{
    const unsigned char payload[] = {0x10, 0x20, 0x30};
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetReadReturn(0, FAKE_WOLFSSL_ERROR_ZERO_RETURN);

    SolidSyslogStream_Send(handle, payload, sizeof(payload));

    LONGS_EQUAL(1, WolfSslTlsFake_FreeCallCount());
    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
}

TEST(SolidSyslogWolfSslStream, SendRefusesTheRecordWhenDataWaitsAheadOfThePeerClosingTheSession)
{
    const unsigned char payload[] = {0x10, 0x20, 0x30};
    const int reads[] = {5, 0};
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetReadReturnSequence(reads, 2, FAKE_WOLFSSL_ERROR_ZERO_RETURN);

    CHECK_FALSE(SolidSyslogStream_Send(handle, payload, sizeof(payload)));
}

TEST(SolidSyslogWolfSslStream, SendWritesToAPeerThatNeverStopsSending)
{
    const unsigned char payload[] = {0x10, 0x20, 0x30};
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetReadReturn(5, 0);

    CHECK_TRUE(SolidSyslogStream_Send(handle, payload, sizeof(payload)));
}

TEST(SolidSyslogWolfSslStream, SendReadsEightTimesSixteenBytesBeforeGivingUpOnAPeerThatNeverStopsSending)
{
    const unsigned char payload[] = {0x10, 0x20, 0x30};
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetReadReturn(5, 0);

    SolidSyslogStream_Send(handle, payload, sizeof(payload));

    LONGS_EQUAL(8, WolfSslTlsFake_ReadCallCount());
    LONGS_EQUAL(16, WolfSslTlsFake_LastReadSz());
}

TEST(SolidSyslogWolfSslStream, SendOnAStreamThatIsNotOpenIsRefusedWithoutWriting)
{
    const unsigned char payload[] = {0x10, 0x20, 0x30};

    CHECK_FALSE(SolidSyslogStream_Send(handle, payload, sizeof(payload)));

    LONGS_EQUAL(0, WolfSslTlsFake_WriteCallCount());
}

/* -------------------------------------------------------------------------
 * Read
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogWolfSslStream, ReadForwardsBufferToSslReadOnTheSession)
{
    unsigned char buffer[8];
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetReadReturn(3, 0);

    SolidSyslogStream_Read(handle, buffer, sizeof(buffer));

    LONGS_EQUAL(1, WolfSslTlsFake_ReadCallCount());
    POINTERS_EQUAL(WolfSslTlsFake_Session(), WolfSslTlsFake_LastReadSsl());
    POINTERS_EQUAL(buffer, WolfSslTlsFake_LastReadData());
    LONGS_EQUAL(sizeof(buffer), WolfSslTlsFake_LastReadSz());
}

TEST(SolidSyslogWolfSslStream, ReadReturnsByteCountWhenSslReadReturnsPositive)
{
    unsigned char buffer[8];
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetReadReturn(5, 0);

    LONGS_EQUAL(5, SolidSyslogStream_Read(handle, buffer, sizeof(buffer)));
}

TEST(SolidSyslogWolfSslStream, ReadReturnsZeroOnWantRead)
{
    unsigned char buffer[8];
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetReadReturn(-1, FAKE_WOLFSSL_ERROR_WANT_READ);

    LONGS_EQUAL(0, SolidSyslogStream_Read(handle, buffer, sizeof(buffer)));
}

TEST(SolidSyslogWolfSslStream, ReadDoesNotCloseOnWantRead)
{
    unsigned char buffer[8];
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetReadReturn(-1, FAKE_WOLFSSL_ERROR_WANT_READ);

    SolidSyslogStream_Read(handle, buffer, sizeof(buffer));

    LONGS_EQUAL(0, WolfSslTlsFake_FreeCallCount());
    LONGS_EQUAL(0, StreamFake_CloseCallCount(transport));
}

TEST(SolidSyslogWolfSslStream, ReadReturnsNegativeOnSslReadError)
{
    unsigned char buffer[8];
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetReadReturn(-1, FAKE_FATAL_ERROR);

    CHECK(SolidSyslogStream_Read(handle, buffer, sizeof(buffer)) < 0);
}

TEST(SolidSyslogWolfSslStream, ReadClosesTheSessionAndTransportOnHardError)
{
    unsigned char buffer[8];
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetReadReturn(-1, FAKE_FATAL_ERROR);

    SolidSyslogStream_Read(handle, buffer, sizeof(buffer));

    LONGS_EQUAL(1, WolfSslTlsFake_FreeCallCount());
    LONGS_EQUAL(1, WolfSslTlsFake_CtxFreeCallCount());
    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
}

TEST(SolidSyslogWolfSslStream, ReadClosesTheSessionWhenThePeerHasClosedIt)
{
    unsigned char buffer[8];
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetReadReturn(0, FAKE_WOLFSSL_ERROR_ZERO_RETURN);

    CHECK(SolidSyslogStream_Read(handle, buffer, sizeof(buffer)) < 0);
    LONGS_EQUAL(1, WolfSslTlsFake_FreeCallCount());
    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
}

/* -------------------------------------------------------------------------
 * The I/O callbacks bridge the Stream contract to wolfSSL's.
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogWolfSslStream, IoSendCallbackForwardsBufferToTransport)
{
    SolidSyslogStream_Open(handle, addr);
    auto* ioSend = WolfSslTlsFake_LastIoSendCallback();
    void* context = WolfSslTlsFake_LastIoWriteCtx();
    char payload[] = {'\xAA', '\xBB', '\xCC'};

    int rc = ioSend(WolfSslTlsFake_Session(), payload, sizeof(payload), context);

    LONGS_EQUAL((int) sizeof(payload), rc);
    LONGS_EQUAL(1, StreamFake_SendCallCount(transport));
    POINTERS_EQUAL(payload, StreamFake_LastSendBuf(transport));
    LONGS_EQUAL(sizeof(payload), StreamFake_LastSendSize(transport));
}

TEST(SolidSyslogWolfSslStream, IoSendCallbackReturnsAGeneralErrorWhenTheTransportFails)
{
    SolidSyslogStream_Open(handle, addr);
    auto* ioSend = WolfSslTlsFake_LastIoSendCallback();
    void* context = WolfSslTlsFake_LastIoWriteCtx();
    char payload[] = {'\xAA'};
    StreamFake_SetSendFails(transport, true);

    int rc = ioSend(WolfSslTlsFake_Session(), payload, sizeof(payload), context);

    LONGS_EQUAL(FAKE_WOLFSSL_CBIO_ERR_GENERAL, rc);
}

TEST(SolidSyslogWolfSslStream, IoRecvCallbackForwardsBufferToTransport)
{
    SolidSyslogStream_Open(handle, addr);
    auto* ioRecv = WolfSslTlsFake_LastIoRecvCallback();
    void* context = WolfSslTlsFake_LastIoReadCtx();
    char buffer[16];
    StreamFake_SetReadReturn(transport, 4);

    int rc = ioRecv(WolfSslTlsFake_Session(), buffer, sizeof(buffer), context);

    LONGS_EQUAL(4, rc);
    LONGS_EQUAL(1, StreamFake_ReadCallCount(transport));
    POINTERS_EQUAL(buffer, StreamFake_LastReadBuf(transport));
    LONGS_EQUAL(sizeof(buffer), StreamFake_LastReadSize(transport));
}

TEST(SolidSyslogWolfSslStream, IoRecvCallbackReturnsWantReadWhenTheTransportWouldBlock)
{
    SolidSyslogStream_Open(handle, addr);
    auto* ioRecv = WolfSslTlsFake_LastIoRecvCallback();
    void* context = WolfSslTlsFake_LastIoReadCtx();
    char buffer[16];
    StreamFake_SetReadReturn(transport, 0);

    int rc = ioRecv(WolfSslTlsFake_Session(), buffer, sizeof(buffer), context);

    LONGS_EQUAL(FAKE_WOLFSSL_CBIO_ERR_WANT_READ, rc);
}

TEST(SolidSyslogWolfSslStream, IoRecvCallbackReturnsAGeneralErrorWhenTheTransportFails)
{
    SolidSyslogStream_Open(handle, addr);
    auto* ioRecv = WolfSslTlsFake_LastIoRecvCallback();
    void* context = WolfSslTlsFake_LastIoReadCtx();
    char buffer[16];
    StreamFake_SetReadReturn(transport, -1);

    int rc = ioRecv(WolfSslTlsFake_Session(), buffer, sizeof(buffer), context);

    LONGS_EQUAL(FAKE_WOLFSSL_CBIO_ERR_GENERAL, rc);
}

/* -------------------------------------------------------------------------
 * Close: close_notify once, the session before the context, the credentials
 * after both, the transport last.
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogWolfSslStream, CloseSendsCloseNotifyOnceOnTheSessionFromOpen)
{
    SolidSyslogStream_Open(handle, addr);

    SolidSyslogStream_Close(handle);

    LONGS_EQUAL(1, WolfSslTlsFake_ShutdownCallCount());
    POINTERS_EQUAL(WolfSslTlsFake_Session(), WolfSslTlsFake_LastShutdownArg());
}

TEST(SolidSyslogWolfSslStream, CloseFreesTheSessionThenTheContextFromOpen)
{
    SolidSyslogStream_Open(handle, addr);

    SolidSyslogStream_Close(handle);

    LONGS_EQUAL(1, WolfSslTlsFake_FreeCallCount());
    POINTERS_EQUAL(WolfSslTlsFake_Session(), WolfSslTlsFake_LastFreeArg());
    LONGS_EQUAL(1, WolfSslTlsFake_CtxFreeCallCount());
    POINTERS_EQUAL(WolfSslTlsFake_Ctx(), WolfSslTlsFake_LastCtxFreeArg());
    LONGS_EQUAL(1, WolfSslTlsFake_FreesSeenAtCtxFree());
}

TEST(SolidSyslogWolfSslStream, CloseTwiceSendsCloseNotifyAndFreesOnce)
{
    SolidSyslogStream_Open(handle, addr);

    SolidSyslogStream_Close(handle);
    SolidSyslogStream_Close(handle);

    LONGS_EQUAL(1, WolfSslTlsFake_ShutdownCallCount());
    LONGS_EQUAL(1, WolfSslTlsFake_FreeCallCount());
    LONGS_EQUAL(1, WolfSslTlsFake_CtxFreeCallCount());
}

TEST(SolidSyslogWolfSslStream, CloseAfterAnInternalCloseFromASendFailureDoesNotDoubleFree)
{
    const unsigned char payload[] = {0x10};
    SolidSyslogStream_Open(handle, addr);
    WolfSslTlsFake_SetWriteReturn(-1);
    SolidSyslogStream_Send(handle, payload, sizeof(payload));

    SolidSyslogStream_Close(handle);

    LONGS_EQUAL(1, WolfSslTlsFake_FreeCallCount());
    LONGS_EQUAL(1, WolfSslTlsFake_CtxFreeCallCount());
}

TEST(SolidSyslogWolfSslStream, CloseWithoutAnOpenTouchesNoSession)
{
    SolidSyslogStream_Close(handle);

    LONGS_EQUAL(0, WolfSslTlsFake_ShutdownCallCount());
    LONGS_EQUAL(0, WolfSslTlsFake_FreeCallCount());
    LONGS_EQUAL(0, WolfSslTlsFake_CtxFreeCallCount());
    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
}

/* -------------------------------------------------------------------------
 * One Release for every Install.
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogWolfSslStream, OpenInstallsTheCredentialsBeforeTheHandshake)
{
    WolfSslCredentialsFake_SetInstallObserver(CaptureNewsAtInstall);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(0, ConnectsSeenAtInstall);
}

TEST(SolidSyslogWolfSslStream, OpenDoesNotHandshakeWhenTheCredentialsCannotInstall)
{
    WolfSslCredentialsFake_SetInstallSucceeds(false);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(0, WolfSslTlsFake_ConnectCallCount());
}

TEST(SolidSyslogWolfSslStream, CloseReleasesTheCredentialsItInstalled)
{
    SolidSyslogStream_Open(handle, addr);

    SolidSyslogStream_Close(handle);

    LONGS_EQUAL(1, WolfSslCredentialsFake_ReleaseCallCount());
}

TEST(SolidSyslogWolfSslStream, AFailedInstallIsStillAnsweredByARelease)
{
    WolfSslCredentialsFake_SetInstallSucceeds(false);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, WolfSslCredentialsFake_ReleaseCallCount());
}

TEST(SolidSyslogWolfSslStream, AnOpenThatFailedLaterIsStillAnsweredByARelease)
{
    ArrangePersistentHandshakeError(FAKE_FATAL_ERROR);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, WolfSslCredentialsFake_ReleaseCallCount());
}

TEST(SolidSyslogWolfSslStream, CloseWithoutAnOpenReleasesNothing)
{
    SolidSyslogStream_Close(handle);

    LONGS_EQUAL(0, WolfSslCredentialsFake_ReleaseCallCount());
}

TEST(SolidSyslogWolfSslStream, CloseTwiceReleasesOnlyOnce)
{
    SolidSyslogStream_Open(handle, addr);

    SolidSyslogStream_Close(handle);
    SolidSyslogStream_Close(handle);

    LONGS_EQUAL(1, WolfSslCredentialsFake_ReleaseCallCount());
}

TEST(SolidSyslogWolfSslStream, CredentialsAreReleasedAfterTheContextIsFreed)
{
    WolfSslCredentialsFake_SetReleaseObserver(CaptureCtxFreesAtRelease);
    SolidSyslogStream_Open(handle, addr);

    SolidSyslogStream_Close(handle);

    LONGS_EQUAL(1, CtxFreesSeenAtRelease);
}

TEST(SolidSyslogWolfSslStream, ASecondOpenInstallsTheCredentialsAgain)
{
    SolidSyslogStream_Open(handle, addr);
    SolidSyslogStream_Close(handle);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(2, WolfSslCredentialsFake_InstallCallCount());
}
