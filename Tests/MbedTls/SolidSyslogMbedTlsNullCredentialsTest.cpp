#include "CppUTest/TestHarness.h"
#include "SolidSyslogMbedTlsCredentialsDefinition.h"
#include "SolidSyslogMbedTlsNullCredentials.h"
#include "SolidSyslogTlsCredentialsInstalled.h"

// clang-format off
TEST_GROUP(SolidSyslogMbedTlsNullCredentials)
{
    struct SolidSyslogMbedTlsCredentials* credentials = nullptr;

    void setup() override
    {
        credentials = SolidSyslogMbedTlsNullCredentials_Get();
    }
};

// clang-format on

TEST(SolidSyslogMbedTlsNullCredentials, GetReturnsNonNull)
{
    CHECK_TRUE(credentials != nullptr);
}

TEST(SolidSyslogMbedTlsNullCredentials, InstallReturnsTrue)
{
    struct SolidSyslogTlsCredentialsInstalled installed = {};
    CHECK_TRUE(credentials->Install(credentials, nullptr, &installed));
}

TEST(SolidSyslogMbedTlsNullCredentials, InstallReportsNoTrustAnchors)
{
    struct SolidSyslogTlsCredentialsInstalled installed = {};
    installed.TrustAnchorsInstalled = true;
    credentials->Install(credentials, nullptr, &installed);
    CHECK_FALSE(installed.TrustAnchorsInstalled);
}

TEST(SolidSyslogMbedTlsNullCredentials, InstallReportsNoFingerprints)
{
    const char* pin = "sha-256:AA";
    struct SolidSyslogTlsCredentialsInstalled installed = {};
    installed.Fingerprints = &pin;
    installed.FingerprintCount = 1;
    credentials->Install(credentials, nullptr, &installed);
    POINTERS_EQUAL(nullptr, installed.Fingerprints);
    UNSIGNED_LONGS_EQUAL(0, installed.FingerprintCount);
}

TEST(SolidSyslogMbedTlsNullCredentials, ReleaseDoesNotCrash)
{
    credentials->Release(credentials);
}
