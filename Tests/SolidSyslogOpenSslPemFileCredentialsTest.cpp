#include "CppUTest/TestHarness.h"

extern "C"
{
#include "OpenSslFake.h"
#include "SolidSyslogOpenSslCredentialsDefinition.h"
#include "SolidSyslogOpenSslNullCredentials.h"
#include "SolidSyslogOpenSslPemFileCredentials.h"
}

// clang-format off
TEST_GROUP(SolidSyslogOpenSslPemFileCredentials)
{
    struct SolidSyslogOpenSslPemFileCredentialsConfig config = {};
    struct SolidSyslogOpenSslCredentials* credentials = nullptr;

    void setup() override
    {
        OpenSslFake_Reset();
        config.CaBundlePath = "ca.pem";
    }

    void teardown() override
    {
        if (credentials != nullptr)
        {
            SolidSyslogOpenSslPemFileCredentials_Destroy(credentials);
        }
    }
};

// clang-format on

TEST(SolidSyslogOpenSslPemFileCredentials, CreateReturnsAPooledHandle)
{
    credentials = SolidSyslogOpenSslPemFileCredentials_Create(&config);

    CHECK_TRUE(credentials != SolidSyslogOpenSslNullCredentials_Get());
}
