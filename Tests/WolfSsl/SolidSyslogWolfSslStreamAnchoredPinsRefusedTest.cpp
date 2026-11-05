#include "CppUTest/TestHarness.h"

extern "C"
{
#include "AddressFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogWolfSslStream.h"
#include "SolidSyslogWolfSslStreamErrors.h"
#include "StreamFake.h"
#include "WolfSslCredentialsFake.h"
#include "WolfSslFake.h"
#include "WolfSslTlsFake.h"
}

#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTlsStreamErrors.h"
#include "TestUtils.h"

using namespace CososoTesting;

/* The stream on a build with neither WOLFSSL_ALWAYS_VERIFY_CB nor
   KEEP_PEER_CERT, which cannot check a pin on a chain that verifies: anchors
   and pins together are refused. This executable is compiled with that body
   selected. */

/* The wolfSSL code the fake answers with, named as in error-crypt.h. */
enum
{
    FAKE_ASN_NO_SIGNER_E = -188
};

static const char* const TEST_SHA256_PIN = "sha-256:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:"
                                           "5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A:5A";

static void NoOpSleep(uint32_t milliseconds)
{
    (void) milliseconds;
}

extern "C" void TestProfile(struct SolidSyslogWolfSslProfile* profile, void* context)
{
    (void) context;
    profile->ServerName = "syslog.example.com";
}

// clang-format off
TEST_GROUP(SolidSyslogWolfSslStreamAnchoredPinsRefused)
{
    struct SolidSyslogStream*             transport = nullptr;
    struct SolidSyslogStream*             handle    = nullptr;
    struct SolidSyslogWolfSslStreamConfig config    = {};

    void setup() override
    {
        WolfSslTlsFake_Reset();
        WolfSslFake_Reset();
        WolfSslCredentialsFake_Reset();
        ErrorHandlerFake_Install(nullptr);
        transport = StreamFake_Create();
        config.Transport = transport;
        config.Sleep = NoOpSleep;
        config.Credentials = WolfSslCredentialsFake_Get();
        config.Profile = TestProfile;
        handle = SolidSyslogWolfSslStream_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogWolfSslStream_Destroy(handle);
        StreamFake_Destroy(transport);
    }
};

// clang-format on

TEST(SolidSyslogWolfSslStreamAnchoredPinsRefused, AnchorsAndPinsTogetherAreRefusedBeforeASessionIsMade)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    CHECK_FALSE(SolidSyslogStream_Open(handle, AddressFake_Get()));

    LONGS_EQUAL(0, WolfSslTlsFake_NewCallCount());
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &SolidSyslogWolfSslStreamErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_STREAM_ERROR_PINS_WITH_ANCHORS_UNSUPPORTED
    );
}

TEST(SolidSyslogWolfSslStreamAnchoredPinsRefused, APinAloneIsStillCheckedInTheCallbackAndConnects)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);
    int errors[] = {FAKE_ASN_NO_SIGNER_E};
    WolfSslTlsFake_SetVerifyErrors(errors, 1);

    CHECK_TRUE(SolidSyslogStream_Open(handle, AddressFake_Get()));
}

TEST(SolidSyslogWolfSslStreamAnchoredPinsRefused, AnchorsAloneStillConnect)
{
    CHECK_TRUE(SolidSyslogStream_Open(handle, AddressFake_Get()));
}

TEST(SolidSyslogWolfSslStreamAnchoredPinsRefused, APinAloneWhoseHandshakeCompletedWithoutAMatchIsRefused)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    CHECK_FALSE(SolidSyslogStream_Open(handle, AddressFake_Get()));

    LONGS_EQUAL(0, WolfSslTlsFake_GetPeerCertificateCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_FINGERPRINT_MISMATCHED, ErrorHandlerFake_LastDetail());
}
