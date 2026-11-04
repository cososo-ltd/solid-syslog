#include "CppUTest/TestHarness.h"
#include "SolidSyslogTlsCredentialsInstalled.h"
#include "SolidSyslogWolfSslCredentialsDefinition.h"
#include "SolidSyslogWolfSslNullCredentials.h"

// clang-format off
TEST_GROUP(SolidSyslogWolfSslNullCredentials)
{
    struct SolidSyslogWolfSslCredentials* credentials = nullptr;

    void setup() override
    {
        credentials = SolidSyslogWolfSslNullCredentials_Get();
    }
};

// clang-format on

TEST(SolidSyslogWolfSslNullCredentials, GetReturnsNonNull)
{
    CHECK_TRUE(credentials != nullptr);
}

TEST(SolidSyslogWolfSslNullCredentials, InstallReturnsTrue)
{
    struct SolidSyslogTlsCredentialsInstalled installed = {};
    CHECK_TRUE(credentials->Install(credentials, nullptr, &installed));
}

TEST(SolidSyslogWolfSslNullCredentials, InstallReportsNoTrustAnchors)
{
    struct SolidSyslogTlsCredentialsInstalled installed = {};
    installed.TrustAnchorsInstalled = true;
    credentials->Install(credentials, nullptr, &installed);
    CHECK_FALSE(installed.TrustAnchorsInstalled);
}

TEST(SolidSyslogWolfSslNullCredentials, InstallReportsNoFingerprints)
{
    const char* pin = "sha-256:AA";
    struct SolidSyslogTlsCredentialsInstalled installed = {};
    installed.Fingerprints = &pin;
    installed.FingerprintCount = 1;
    credentials->Install(credentials, nullptr, &installed);
    POINTERS_EQUAL(nullptr, installed.Fingerprints);
    UNSIGNED_LONGS_EQUAL(0, installed.FingerprintCount);
}

TEST(SolidSyslogWolfSslNullCredentials, ReleaseDoesNotCrash)
{
    credentials->Release(credentials);
}
