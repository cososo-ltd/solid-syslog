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
