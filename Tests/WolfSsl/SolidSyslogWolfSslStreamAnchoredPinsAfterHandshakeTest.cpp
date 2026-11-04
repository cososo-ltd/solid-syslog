#include "CppUTest/TestHarness.h"

extern "C"
{
#include "AddressFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogWolfSslStream.h"
#include "StreamFake.h"
#include "WolfSslCredentialsFake.h"
#include "WolfSslFake.h"
#include "WolfSslTlsFake.h"
}

#include "SolidSyslogTlsStreamErrors.h"

/* The stream on a build with KEEP_PEER_CERT and without
   WOLFSSL_ALWAYS_VERIFY_CB, where the callback never reaches a leaf that
   verifies: a pin beside anchors is checked on the leaf the library kept, once
   the handshake is done. This executable is compiled with that body selected. */

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
TEST_GROUP(SolidSyslogWolfSslStreamAnchoredPinsAfterHandshake)
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

TEST(SolidSyslogWolfSslStreamAnchoredPinsAfterHandshake, AKeptLeafThatMatchesAPinConnects)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    CHECK_TRUE(SolidSyslogStream_Open(handle, AddressFake_Get()));
}

TEST(SolidSyslogWolfSslStreamAnchoredPinsAfterHandshake, AKeptLeafThatMatchesNoPinIsRefusedAndTheSessionClosed)
{
    const char* const pins[] = {"sha-256:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:"
                                "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00"};
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    CHECK_FALSE(SolidSyslogStream_Open(handle, AddressFake_Get()));

    LONGS_EQUAL(1, WolfSslTlsFake_FreeCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_FINGERPRINT_MISMATCHED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWolfSslStreamAnchoredPinsAfterHandshake, TheKeptCertificateIsFreed)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    (void) SolidSyslogStream_Open(handle, AddressFake_Get());

    LONGS_EQUAL(1, WolfSslTlsFake_X509FreeCallCount());
    POINTERS_EQUAL(WolfSslTlsFake_PeerCertificate(), WolfSslTlsFake_LastX509FreeArg());
}

TEST(SolidSyslogWolfSslStreamAnchoredPinsAfterHandshake, NoKeptCertificateIsAPinWhoseDigestCannotBeTaken)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);
    WolfSslTlsFake_SetPeerCertificateReturnsNull(true);

    CHECK_FALSE(SolidSyslogStream_Open(handle, AddressFake_Get()));

    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_FINGERPRINT_DIGEST_UNAVAILABLE, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWolfSslStreamAnchoredPinsAfterHandshake, AKeptCertificateWithNoDerIsAPinWhoseDigestCannotBeTaken)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);
    WolfSslTlsFake_SetX509GetDerReturnsNull(true);

    CHECK_FALSE(SolidSyslogStream_Open(handle, AddressFake_Get()));

    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_ERROR_FINGERPRINT_DIGEST_UNAVAILABLE, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogWolfSslStreamAnchoredPinsAfterHandshake, TheLeafDigestedIsTheOneTheSessionKept)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);

    (void) SolidSyslogStream_Open(handle, AddressFake_Get());

    POINTERS_EQUAL(WolfSslTlsFake_Session(), WolfSslTlsFake_LastGetPeerCertificateSsl());
    POINTERS_EQUAL(WolfSslTlsFake_LeafDer(), WolfSslFake_LastHashData());
    UNSIGNED_LONGS_EQUAL(WolfSslTlsFake_LeafDerLength(), WolfSslFake_LastHashDataLen());
}

TEST(SolidSyslogWolfSslStreamAnchoredPinsAfterHandshake, APinTheCallbackMatchedIsNotCheckedAgain)
{
    const char* const pins[] = {TEST_SHA256_PIN};
    WolfSslCredentialsFake_SetTrustAnchorsInstalled(false);
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);
    int errors[] = {FAKE_ASN_NO_SIGNER_E};
    WolfSslTlsFake_SetVerifyErrors(errors, 1);

    CHECK_TRUE(SolidSyslogStream_Open(handle, AddressFake_Get()));

    LONGS_EQUAL(0, WolfSslTlsFake_GetPeerCertificateCallCount());
}

TEST(SolidSyslogWolfSslStreamAnchoredPinsAfterHandshake, AnUnpinnedPeerHasNothingCheckedAfterTheHandshake)
{
    CHECK_TRUE(SolidSyslogStream_Open(handle, AddressFake_Get()));

    LONGS_EQUAL(0, WolfSslTlsFake_GetPeerCertificateCallCount());
}

TEST(SolidSyslogWolfSslStreamAnchoredPinsAfterHandshake, AVerifiedLeafIsLeftToTheCheckAfterTheHandshake)
{
    const char* const pins[] = {"sha-256:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:"
                                "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00"};
    WolfSslCredentialsFake_SetFingerprints(pins, 1U);
    int errors[] = {0};
    WolfSslTlsFake_SetVerifyErrors(errors, 1);

    CHECK_FALSE(SolidSyslogStream_Open(handle, AddressFake_Get()));

    LONGS_EQUAL(1, WolfSslTlsFake_LastVerifyCallbackResult());
    LONGS_EQUAL(1, WolfSslTlsFake_GetPeerCertificateCallCount());
}
