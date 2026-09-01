#include "CppUTest/TestHarness.h"

extern "C"
{
#include "ErrorHandlerFake.h"
#include "OpenSslFake.h"
#include "SolidSyslogOpenSslCredentialsDefinition.h"
#include "SolidSyslogOpenSslNullCredentials.h"
#include "SolidSyslogOpenSslPemFileCredentials.h"
#include "SolidSyslogOpenSslPemFileCredentialsErrors.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
}

#include "SolidSyslogErrorCategory.h"
#include "TestUtils.h"

using namespace CososoTesting;

// clang-format off
TEST_GROUP(SolidSyslogOpenSslPemFileCredentials)
{
    struct SolidSyslogOpenSslPemFileCredentialsConfig config = {};
    struct SolidSyslogOpenSslCredentials* credentials = nullptr;
    struct SolidSyslogOpenSslCredentials* pooled[SOLIDSYSLOG_TLS_CREDENTIALS_POOL_SIZE] = {};
    struct SolidSyslogOpenSslCredentials* overflow = nullptr;

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
        for (auto* slot : pooled)
        {
            if (slot != nullptr)
            {
                SolidSyslogOpenSslPemFileCredentials_Destroy(slot);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogOpenSslPemFileCredentials_Destroy(overflow);
        }
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogOpenSslPemFileCredentials_Create(&config);
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

TEST(SolidSyslogOpenSslPemFileCredentials, CreateBeyondThePoolReturnsTheNullCredentials)
{
    FillPool();

    overflow = SolidSyslogOpenSslPemFileCredentials_Create(&config);

    POINTERS_EQUAL(SolidSyslogOpenSslNullCredentials_Get(), overflow);
    overflow = nullptr;
}

TEST(SolidSyslogOpenSslPemFileCredentials, CreateBeyondThePoolReportsExhaustion)
{
    FillPool();
    ErrorHandlerFake_Install(nullptr);

    overflow = SolidSyslogOpenSslPemFileCredentials_Create(&config);
    overflow = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
        &OpenSslPemFileCredentialsErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_OPENSSL_PEM_FILE_CREDENTIALS_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogOpenSslPemFileCredentials, DestroyingAHandleThePoolDoesNotOwnIsReported)
{
    struct SolidSyslogOpenSslCredentials stranger = {};
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogOpenSslPemFileCredentials_Destroy(&stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
        &OpenSslPemFileCredentialsErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_OPENSSL_PEM_FILE_CREDENTIALS_ERROR_UNKNOWN_DESTROY
    );
}
