#include "CppUTest/TestHarness.h"

#include "ErrorHandlerFake.h"
#include "ErrorHandlerFakeEx.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "TestUtils.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros

// clang-format off
TEST_GROUP(SolidSyslogError)
{
    // cppcheck-suppress unreadVariable -- read via context-propagation test through CppUTest macros
    int sentinel = 0;

    void teardown() override
    {
        ErrorHandlerFake_Uninstall();
    }
};

// clang-format on

TEST(SolidSyslogError, ErrorWithDefaultHandlerDoesNotCrash)
{
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, "test message");
}

TEST(SolidSyslogError, InstalledHandlerReceivesSeverityMessageAndContext)
{
    ErrorHandlerFake_Install(&sentinel);

    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, "warning message");

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL("warning message", ErrorHandlerFake_LastMessage());
    POINTERS_EQUAL(&sentinel, ErrorHandlerFake_LastContext());
}

TEST(SolidSyslogError, SetErrorHandlerWithNullHandlerRestoresDefault)
{
    ErrorHandlerFake_Install(&sentinel);

    SolidSyslog_SetErrorHandler(nullptr, &sentinel);
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, "should not be observed");

    CHECK_NOTHING_REPORTED();
}

// clang-format off
TEST_GROUP(SolidSyslogErrorEx)
{
    // cppcheck-suppress unreadVariable -- read via context-propagation test through CppUTest macros
    int sentinel = 0;
};

// clang-format on

TEST(SolidSyslogErrorEx, ErrorExWithDefaultHandlerDoesNotCrash)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    SolidSyslog_ErrorEx(SOLIDSYSLOG_SEVERITY_ERROR, &source, 0U);
}

TEST(SolidSyslogErrorEx, InstalledHandlerReceivesSeveritySourceCodeAndContext)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    ErrorHandlerFakeEx_Install(&sentinel);

    SolidSyslog_ErrorEx(SOLIDSYSLOG_SEVERITY_WARNING, &source, 7U);

    CALLED_FAKE(ErrorHandlerFakeEx_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFakeEx_LastSeverity());
    POINTERS_EQUAL(&source, ErrorHandlerFakeEx_LastSource());
    UNSIGNED_LONGS_EQUAL(7U, ErrorHandlerFakeEx_LastCode());
    POINTERS_EQUAL(&sentinel, ErrorHandlerFakeEx_LastContext());
}

TEST(SolidSyslogErrorEx, SetErrorHandlerExWithNullHandlerRestoresDefault)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    ErrorHandlerFakeEx_Install(&sentinel);

    SolidSyslog_SetErrorHandlerEx(nullptr, &sentinel);
    SolidSyslog_ErrorEx(SOLIDSYSLOG_SEVERITY_ERROR, &source, 0U);

    CALLED_FAKE(ErrorHandlerFakeEx_Handle, NEVER);
}
