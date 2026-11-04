#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stddef.h>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTlsCredentialsInstalled.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWolfSslCredentialsDefinition.h"
#include "SolidSyslogWolfSslNullCredentials.h"
#include "SolidSyslogWolfSslPemBufferCredentials.h"
#include "SolidSyslogWolfSslPemBufferCredentialsErrors.h"
#include "WolfSslTlsFake.h"
}

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogTlsCredentialsErrors.h"
#include "TestUtils.h"

using namespace CososoTesting;

/* wolfSSL return values and codes as the fake answers them; the names are
   those of ssl.h and error-crypt.h. */
enum
{
    FAKE_WOLFSSL_FAILURE = 0,
    FAKE_WOLFSSL_SUCCESS = 1,
    FAKE_WOLFSSL_FILETYPE_PEM = 1,
    FAKE_MEMORY_E = -125,
    FAKE_ASN_NO_PEM_HEADER = -162
};

/* Stands in for PEM the test never parses: the fake captures the address and
   the extent, and nothing reads the bytes. */
static const unsigned char TEST_CA_PEM[] = "-----BEGIN CERTIFICATE-----\nCA\n-----END CERTIFICATE-----\n";
static const unsigned char TEST_CLIENT_CERT_PEM[] = "-----BEGIN CERTIFICATE-----\nCL\n-----END CERTIFICATE-----\n";
static const unsigned char TEST_CLIENT_KEY_PEM[] = "-----BEGIN EC PRIVATE KEY-----\nK\n-----END EC PRIVATE KEY-----\n";
static const char* const TEST_PINS[] = {"sha-256:AA:BB"};

#define CHECK_NULL_CREDENTIALS(handle) POINTERS_EQUAL(SolidSyslogWolfSslNullCredentials_Get(), (handle))

// clang-format off
TEST_GROUP(SolidSyslogWolfSslPemBufferCredentials)
{
    struct SolidSyslogWolfSslPemBufferCredentialsConfig config      = {};
    struct SolidSyslogWolfSslCredentials*                credentials = nullptr;
    struct SolidSyslogTlsCredentialsInstalled            installed   = {};

    void setup() override
    {
        WolfSslTlsFake_Reset();
        ErrorHandlerFake_Install(nullptr);
        config.CaPem.Bytes = TEST_CA_PEM;
        config.CaPem.Length = sizeof(TEST_CA_PEM) - 1U;
        credentials = SolidSyslogWolfSslPemBufferCredentials_Create(&config);
        installed.TrustAnchorsInstalled = true;
    }

    void teardown() override
    {
        SolidSyslogWolfSslPemBufferCredentials_Destroy(credentials);
        ConfigLockFake_Uninstall();
    }

    void ReCreateWithUpdatedConfig()
    {
        SolidSyslogWolfSslPemBufferCredentials_Destroy(credentials);
        ErrorHandlerFake_Install(nullptr);
        credentials = SolidSyslogWolfSslPemBufferCredentials_Create(&config);
    }

    void GivenAClientCredential()
    {
        config.ClientCertPem.Bytes = TEST_CLIENT_CERT_PEM;
        config.ClientCertPem.Length = sizeof(TEST_CLIENT_CERT_PEM) - 1U;
        config.ClientKeyPem.Bytes = TEST_CLIENT_KEY_PEM;
        config.ClientKeyPem.Length = sizeof(TEST_CLIENT_KEY_PEM) - 1U;
        ReCreateWithUpdatedConfig();
    }

    bool Install()
    {
        return credentials->Install(credentials, WolfSslTlsFake_Ctx(), &installed);
    }
};

// clang-format on

TEST(SolidSyslogWolfSslPemBufferCredentials, CreateReturnsAPooledHandle)
{
    CHECK_TRUE(credentials != nullptr);
    CHECK_TRUE(credentials != SolidSyslogWolfSslNullCredentials_Get());
}

TEST(SolidSyslogWolfSslPemBufferCredentials, CreateWithNullConfigReturnsTheNullCredentials)
{
    struct SolidSyslogWolfSslCredentials* fallback = SolidSyslogWolfSslPemBufferCredentials_Create(nullptr);

    CHECK_NULL_CREDENTIALS(fallback);
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogWolfSslPemBufferCredentialsErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_NULL_CONFIG
    );
}

TEST(SolidSyslogWolfSslPemBufferCredentials, CreateBeyondThePoolReportsExhaustion)
{
    struct SolidSyslogWolfSslCredentials* extra[SOLIDSYSLOG_TLS_CREDENTIALS_POOL_SIZE] = {};
    for (auto*& slot : extra)
    {
        slot = SolidSyslogWolfSslPemBufferCredentials_Create(&config);
    }

    CHECK_NULL_CREDENTIALS(extra[SOLIDSYSLOG_TLS_CREDENTIALS_POOL_SIZE - 1U]);
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogWolfSslPemBufferCredentialsErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_POOL_EXHAUSTED
    );
    for (auto* slot : extra)
    {
        if (slot != SolidSyslogWolfSslNullCredentials_Get())
        {
            SolidSyslogWolfSslPemBufferCredentials_Destroy(slot);
        }
    }
}

TEST(SolidSyslogWolfSslPemBufferCredentials, CreateAcquiresAndReleasesTheConfigLock)
{
    SolidSyslogWolfSslPemBufferCredentials_Destroy(credentials);
    ConfigLockFake_Install();

    credentials = SolidSyslogWolfSslPemBufferCredentials_Create(&config);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWolfSslPemBufferCredentials, DestroyingAHandleThePoolDoesNotOwnIsReported)
{
    struct SolidSyslogWolfSslCredentials stranger = {};

    SolidSyslogWolfSslPemBufferCredentials_Destroy(&stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogWolfSslPemBufferCredentialsErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogWolfSslPemBufferCredentials, InstallLoadsTheTrustAnchorsOntoTheContextAsPem)
{
    Install();

    LONGS_EQUAL(1, WolfSslTlsFake_LoadVerifyBufferCallCount());
    POINTERS_EQUAL(WolfSslTlsFake_Ctx(), WolfSslTlsFake_LastLoadVerifyBufferCtx());
    POINTERS_EQUAL(TEST_CA_PEM, WolfSslTlsFake_LastLoadVerifyBufferIn());
    LONGS_EQUAL(sizeof(TEST_CA_PEM) - 1U, WolfSslTlsFake_LastLoadVerifyBufferSz());
    LONGS_EQUAL(FAKE_WOLFSSL_FILETYPE_PEM, WolfSslTlsFake_LastLoadVerifyBufferFormat());
}

TEST(SolidSyslogWolfSslPemBufferCredentials, InstallReportsThatTheTrustAnchorsAreInstalled)
{
    installed.TrustAnchorsInstalled = false;

    CHECK_TRUE(Install());

    CHECK_TRUE(installed.TrustAnchorsInstalled);
}

TEST(SolidSyslogWolfSslPemBufferCredentials, InstallReportsTrustAnchorsItCannotParse)
{
    WolfSslTlsFake_SetLoadVerifyBufferReturn(FAKE_ASN_NO_PEM_HEADER);

    CHECK_FALSE(Install());

    CHECK_FALSE(installed.TrustAnchorsInstalled);
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &SolidSyslogWolfSslPemBufferCredentialsErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_TRUST_ANCHORS_NOT_PARSED
    );
}

TEST(SolidSyslogWolfSslPemBufferCredentials, InstallWithoutTrustAnchorsLoadsNothingAndStillSucceeds)
{
    config.CaPem.Bytes = nullptr;
    config.CaPem.Length = 0U;
    ReCreateWithUpdatedConfig();

    CHECK_TRUE(Install());

    CHECK_FALSE(installed.TrustAnchorsInstalled);
    LONGS_EQUAL(0, WolfSslTlsFake_LoadVerifyBufferCallCount());
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogWolfSslPemBufferCredentials, InstallReportsNoPeerFingerprints)
{
    installed.Fingerprints = TEST_PINS;
    installed.FingerprintCount = 1U;

    Install();

    POINTERS_EQUAL(nullptr, installed.Fingerprints);
    UNSIGNED_LONGS_EQUAL(0U, installed.FingerprintCount);
}

TEST(SolidSyslogWolfSslPemBufferCredentials, InstallPresentsTheClientCredentialAsPem)
{
    GivenAClientCredential();

    CHECK_TRUE(Install());

    LONGS_EQUAL(1, WolfSslTlsFake_UseCertificateChainBufferCallCount());
    POINTERS_EQUAL(TEST_CLIENT_CERT_PEM, WolfSslTlsFake_LastUseCertificateChainBufferIn());
    LONGS_EQUAL(sizeof(TEST_CLIENT_CERT_PEM) - 1U, WolfSslTlsFake_LastUseCertificateChainBufferSz());
    LONGS_EQUAL(1, WolfSslTlsFake_UsePrivateKeyBufferCallCount());
    POINTERS_EQUAL(TEST_CLIENT_KEY_PEM, WolfSslTlsFake_LastUsePrivateKeyBufferIn());
    LONGS_EQUAL(sizeof(TEST_CLIENT_KEY_PEM) - 1U, WolfSslTlsFake_LastUsePrivateKeyBufferSz());
    LONGS_EQUAL(FAKE_WOLFSSL_FILETYPE_PEM, WolfSslTlsFake_LastUsePrivateKeyBufferFormat());
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogWolfSslPemBufferCredentials, InstallWithoutAClientCredentialPresentsNone)
{
    Install();

    LONGS_EQUAL(0, WolfSslTlsFake_UseCertificateChainBufferCallCount());
    LONGS_EQUAL(0, WolfSslTlsFake_UsePrivateKeyBufferCallCount());
}

TEST(SolidSyslogWolfSslPemBufferCredentials, InstallReportsAHalfSuppliedClientCredential)
{
    config.ClientKeyPem.Bytes = TEST_CLIENT_KEY_PEM;
    config.ClientKeyPem.Length = sizeof(TEST_CLIENT_KEY_PEM) - 1U;
    ReCreateWithUpdatedConfig();

    CHECK_TRUE(Install());

    LONGS_EQUAL(0, WolfSslTlsFake_UsePrivateKeyBufferCallCount());
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogWolfSslPemBufferCredentialsErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_CLIENT_CREDENTIAL_INCOMPLETE
    );
}

TEST(SolidSyslogWolfSslPemBufferCredentials, InstallReportsAClientCertificateItCannotParse)
{
    GivenAClientCredential();
    WolfSslTlsFake_SetUseCertificateChainBufferReturn(FAKE_ASN_NO_PEM_HEADER);

    CHECK_TRUE(Install());

    LONGS_EQUAL(0, WolfSslTlsFake_UsePrivateKeyBufferCallCount());
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogWolfSslPemBufferCredentialsErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_CLIENT_CREDENTIAL_NOT_PARSED
    );
}

TEST(SolidSyslogWolfSslPemBufferCredentials, InstallReportsAClientKeyItCannotParse)
{
    GivenAClientCredential();
    WolfSslTlsFake_SetUsePrivateKeyBufferReturn(FAKE_ASN_NO_PEM_HEADER);

    CHECK_TRUE(Install());

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogWolfSslPemBufferCredentialsErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_CLIENT_CREDENTIAL_NOT_PARSED
    );
}

TEST(SolidSyslogWolfSslPemBufferCredentials, InstallReportsAClientCredentialTheLibraryCouldNotHold)
{
    GivenAClientCredential();
    WolfSslTlsFake_SetUsePrivateKeyBufferReturn(FAKE_MEMORY_E);

    CHECK_TRUE(Install());

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogWolfSslPemBufferCredentialsErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_CLIENT_CREDENTIAL_NOT_INSTALLED
    );
}

TEST(SolidSyslogWolfSslPemBufferCredentials, InstallReportsATrustAnchorBufferWithNoExtent)
{
    config.CaPem.Length = 0U;
    ReCreateWithUpdatedConfig();

    CHECK_FALSE(Install());

    LONGS_EQUAL(0, WolfSslTlsFake_LoadVerifyBufferCallCount());
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &SolidSyslogWolfSslPemBufferCredentialsErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_TRUST_ANCHORS_NOT_PARSED
    );
}

TEST(SolidSyslogWolfSslPemBufferCredentials, InstallReportsAClientCredentialWithNoExtent)
{
    GivenAClientCredential();
    config.ClientKeyPem.Length = 0U;
    ReCreateWithUpdatedConfig();

    CHECK_TRUE(Install());

    LONGS_EQUAL(0, WolfSslTlsFake_UseCertificateChainBufferCallCount());
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogWolfSslPemBufferCredentialsErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_CLIENT_CREDENTIAL_NOT_PARSED
    );
}

TEST(SolidSyslogWolfSslPemBufferCredentials, ReleaseIsSafeWithOrWithoutAnInstall)
{
    credentials->Release(credentials);
    Install();
    credentials->Release(credentials);
    credentials->Release(credentials);
}

TEST(SolidSyslogWolfSslPemBufferCredentials, ASecondInstallLoadsAgain)
{
    Install();
    credentials->Release(credentials);

    Install();

    LONGS_EQUAL(2, WolfSslTlsFake_LoadVerifyBufferCallCount());
}

TEST(SolidSyslogWolfSslPemBufferCredentials, TheErrorSourceNamesItsDetailsThroughTlsCredentialsErrors)
{
    POINTERS_EQUAL(SolidSyslogTlsCredentialsErrors_Name, SolidSyslogWolfSslPemBufferCredentialsErrorSource.DetailName);
}
