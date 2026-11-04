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

/* The stream on a build with KEEP_PEER_CERT and without
   WOLFSSL_ALWAYS_VERIFY_CB, where the callback never reaches a leaf that
   verifies: a pin beside anchors is checked on the leaf the library kept, once
   the handshake is done. This executable is compiled with that body selected. */

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
