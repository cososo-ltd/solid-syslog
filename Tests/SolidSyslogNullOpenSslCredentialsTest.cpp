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
