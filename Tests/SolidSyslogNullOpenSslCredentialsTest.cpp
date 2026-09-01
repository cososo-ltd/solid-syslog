#include "CppUTest/TestHarness.h"
#include "SolidSyslogNullOpenSslCredentials.h"
#include "SolidSyslogOpenSslCredentialsDefinition.h"

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
