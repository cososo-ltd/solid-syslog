#include "CppUTest/TestHarness.h"
#include "SolidSyslogOpenSslNullCredentials.h"
#include "SolidSyslogOpenSslCredentialsDefinition.h"
#include "SolidSyslogTlsCredentialsInstalled.h"

// clang-format off
TEST_GROUP(SolidSyslogOpenSslNullCredentials)
{
    struct SolidSyslogOpenSslCredentials* credentials = nullptr;

    void setup() override
    {
        credentials = SolidSyslogOpenSslNullCredentials_Get();
    }
};

// clang-format on

TEST(SolidSyslogOpenSslNullCredentials, GetReturnsNonNull)
{
    CHECK_TRUE(credentials != nullptr);
}

TEST(SolidSyslogOpenSslNullCredentials, InstallReturnsTrue)
{
    struct SolidSyslogTlsCredentialsInstalled installed;
    CHECK_TRUE(credentials->Install(credentials, nullptr, &installed));
}

TEST(SolidSyslogOpenSslNullCredentials, InstallReportsNoTrustAnchors)
{
    struct SolidSyslogTlsCredentialsInstalled installed;
    installed.TrustAnchorsInstalled = true;
    credentials->Install(credentials, nullptr, &installed);
    CHECK_FALSE(installed.TrustAnchorsInstalled);
}

TEST(SolidSyslogOpenSslNullCredentials, InstallReportsNoFingerprints)
{
    const char* pin = "sha-256:AA";
    struct SolidSyslogTlsCredentialsInstalled installed;
    installed.Fingerprints = &pin;
    installed.FingerprintCount = 1;
    credentials->Install(credentials, nullptr, &installed);
    POINTERS_EQUAL(nullptr, installed.Fingerprints);
    UNSIGNED_LONGS_EQUAL(0, installed.FingerprintCount);
}

TEST(SolidSyslogOpenSslNullCredentials, ReleaseDoesNotCrash)
{
    credentials->Release(credentials);
}
