#include "CppUTest/TestHarness.h"
#include "SolidSyslogMbedTlsCredentialsDefinition.h"
#include "SolidSyslogNullMbedTlsCredentials.h"
#include "SolidSyslogTlsCredentialsInstalled.h"

// clang-format off
TEST_GROUP(SolidSyslogNullMbedTlsCredentials)
{
    struct SolidSyslogMbedTlsCredentials* credentials = nullptr;

    void setup() override
    {
        credentials = SolidSyslogNullMbedTlsCredentials_Get();
    }
};

// clang-format on

TEST(SolidSyslogNullMbedTlsCredentials, GetReturnsNonNull)
{
    CHECK_TRUE(credentials != nullptr);
}

TEST(SolidSyslogNullMbedTlsCredentials, InstallReturnsTrue)
{
    struct SolidSyslogTlsCredentialsInstalled installed;
    CHECK_TRUE(credentials->Install(credentials, nullptr, &installed));
}

TEST(SolidSyslogNullMbedTlsCredentials, InstallReportsNoTrustAnchors)
{
    struct SolidSyslogTlsCredentialsInstalled installed;
    installed.TrustAnchorsInstalled = true;
    credentials->Install(credentials, nullptr, &installed);
    CHECK_FALSE(installed.TrustAnchorsInstalled);
}

TEST(SolidSyslogNullMbedTlsCredentials, InstallReportsNoFingerprints)
{
    const char* pin = "sha-256:AA";
    struct SolidSyslogTlsCredentialsInstalled installed;
    installed.Fingerprints = &pin;
    installed.FingerprintCount = 1;
    credentials->Install(credentials, nullptr, &installed);
    POINTERS_EQUAL(nullptr, installed.Fingerprints);
    UNSIGNED_LONGS_EQUAL(0, installed.FingerprintCount);
}
