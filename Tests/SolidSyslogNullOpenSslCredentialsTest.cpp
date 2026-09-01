#include "CppUTest/TestHarness.h"
#include "SolidSyslogNullOpenSslCredentials.h"
#include "SolidSyslogOpenSslCredentialsDefinition.h"
#include "SolidSyslogTlsCredentialsInstalled.h"

// clang-format off
TEST_GROUP(SolidSyslogNullOpenSslCredentials)
{
    struct SolidSyslogOpenSslCredentials* credentials = nullptr;

    void setup() override
    {
        credentials = SolidSyslogNullOpenSslCredentials_Get();
    }
};

// clang-format on

TEST(SolidSyslogNullOpenSslCredentials, GetReturnsNonNull)
{
    CHECK_TRUE(credentials != nullptr);
}

TEST(SolidSyslogNullOpenSslCredentials, InstallReturnsTrue)
{
    struct SolidSyslogTlsCredentialsInstalled installed;
    CHECK_TRUE(credentials->Install(credentials, nullptr, &installed));
}

TEST(SolidSyslogNullOpenSslCredentials, InstallReportsNoTrustAnchors)
{
    struct SolidSyslogTlsCredentialsInstalled installed;
    installed.TrustAnchorsInstalled = true;
    credentials->Install(credentials, nullptr, &installed);
    CHECK_FALSE(installed.TrustAnchorsInstalled);
}

TEST(SolidSyslogNullOpenSslCredentials, InstallReportsNoFingerprints)
{
    const char* pin = "sha-256:AA";
    struct SolidSyslogTlsCredentialsInstalled installed;
    installed.Fingerprints = &pin;
    installed.FingerprintCount = 1;
    credentials->Install(credentials, nullptr, &installed);
    POINTERS_EQUAL(nullptr, installed.Fingerprints);
    UNSIGNED_LONGS_EQUAL(0, installed.FingerprintCount);
}

TEST(SolidSyslogNullOpenSslCredentials, ReleaseDoesNotCrash)
{
    credentials->Release(credentials);
}
