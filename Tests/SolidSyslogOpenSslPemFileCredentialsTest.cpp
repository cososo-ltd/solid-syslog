#include "CppUTest/TestHarness.h"

extern "C"
{
#include "ErrorHandlerFake.h"
#include "OpenSslFake.h"
#include "SolidSyslogOpenSslCredentialsDefinition.h"
#include "SolidSyslogOpenSslNullCredentials.h"
#include "SolidSyslogOpenSslPemFileCredentials.h"
#include "SolidSyslogOpenSslPemFileCredentialsErrors.h"
#include "SolidSyslogPrival.h"
}

#include "SolidSyslogErrorCategory.h"
#include "TestUtils.h"

using namespace CososoTesting;

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

TEST(SolidSyslogOpenSslPemFileCredentials, CreateWithNullConfigReturnsTheNullCredentials)
{
    credentials = SolidSyslogOpenSslPemFileCredentials_Create(nullptr);

    POINTERS_EQUAL(SolidSyslogOpenSslNullCredentials_Get(), credentials);
    credentials = nullptr;
}

TEST(SolidSyslogOpenSslPemFileCredentials, CreateWithNullConfigReportsBadConfig)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogOpenSslPemFileCredentials_Create(nullptr);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &OpenSslPemFileCredentialsErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_OPENSSL_PEM_FILE_CREDENTIALS_ERROR_NULL_CONFIG
    );
}
