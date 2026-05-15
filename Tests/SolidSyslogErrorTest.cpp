#include "CppUTest/TestHarness.h"

#include "ErrorHandlerFake.h"
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
    SolidSyslog_Error(SolidSyslogSeverity_Error, "test message");
}

TEST(SolidSyslogError, InstalledHandlerReceivesSeverityMessageAndContext)
{
    ErrorHandlerFake_Install(&sentinel);

    SolidSyslog_Error(SolidSyslogSeverity_Warning, "warning message");

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SolidSyslogSeverity_Warning, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL("warning message", ErrorHandlerFake_LastMessage());
    POINTERS_EQUAL(&sentinel, ErrorHandlerFake_LastContext());
}

TEST(SolidSyslogError, SetErrorHandlerWithNullHandlerRestoresDefault)
{
    ErrorHandlerFake_Install(&sentinel);

    SolidSyslog_SetErrorHandler(nullptr, &sentinel);
    SolidSyslog_Error(SolidSyslogSeverity_Error, "should not be observed");

    CHECK_NOTHING_REPORTED();
}
