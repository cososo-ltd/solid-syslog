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
#include "WolfSslTlsFake.h"
}

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogTlsStreamCategories.h"
#include "SolidSyslogTlsStreamErrors.h"
#include "TestUtils.h"

using namespace CososoTesting;

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

extern "C" void CaptureNewsAtInstall(void)
{
    NewsSeenAtInstall = WolfSslTlsFake_NewCallCount();
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
        WolfSslCredentialsFake_Reset();
        ErrorHandlerFake_Install(nullptr);
        FakeGetHandshakeTimeoutMs_Reset();
        FakeVersion_Reset();
        FakeProfile_Reset();
        FakeProfile_Value.ServerName = "syslog.example.com";
        NoOpSleepCallCount = 0;
        g_lastSleepMs = 0;
        NewsSeenAtInstall = -1;
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
