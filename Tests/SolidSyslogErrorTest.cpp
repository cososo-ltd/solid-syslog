#include "CppUTest/TestHarness.h"

#include "SolidSyslogConfig.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogPrival.h"
#include "TestUtils.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
                               // macros

static int                       handlerCallCount;
static enum SolidSyslog_Severity capturedSeverity;
static const char*               capturedMessage;
static void*                     capturedContext;

static void TestErrorHandler(void* context, enum SolidSyslog_Severity severity, const char* message)
{
    handlerCallCount++;
    capturedSeverity = severity;
    capturedMessage  = message;
    capturedContext  = context;
}

// NOLINTBEGIN(cppcoreguidelines-macro-usage) -- macros preserve __FILE__/__LINE__ in test failure output
#define CHECK_EXPECTED_SEVERITY(expected) LONGS_EQUAL((expected), capturedSeverity)
#define CHECK_EXPECTED_MESSAGE(expected) STRCMP_EQUAL((expected), capturedMessage)
#define CHECK_EXPECTED_CONTEXT(expected) POINTERS_EQUAL((expected), capturedContext)

// NOLINTEND(cppcoreguidelines-macro-usage)

// clang-format off
TEST_GROUP(SolidSyslogError)
{
    int sentinel = 0;

    void setup() override
    {
        handlerCallCount = 0;
        capturedSeverity = SOLIDSYSLOG_SEVERITY_DEBUG;
        capturedMessage  = nullptr;
        capturedContext  = nullptr;
    }

    void teardown() override
    {
        SolidSyslog_SetErrorHandler(nullptr, nullptr);
    }

    void installHandler()
    {
        SolidSyslog_SetErrorHandler(TestErrorHandler, &sentinel);
    }
};

// clang-format on

TEST(SolidSyslogError, ErrorWithDefaultHandlerDoesNotCrash)
{
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, "test message");
}

TEST(SolidSyslogError, InstalledHandlerReceivesSeverityMessageAndContext)
{
    installHandler();
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, "warning message");

    CALLED_FUNCTION(handler, ONCE);
    CHECK_EXPECTED_SEVERITY(SOLIDSYSLOG_SEVERITY_WARNING);
    CHECK_EXPECTED_MESSAGE("warning message");
    CHECK_EXPECTED_CONTEXT(&sentinel);
}

TEST(SolidSyslogError, SetErrorHandlerWithNullHandlerRestoresDefault)
{
    installHandler();

    SolidSyslog_SetErrorHandler(nullptr, &sentinel);
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERR, "should not be observed");

    CALLED_FUNCTION(handler, NEVER);
}

TEST(SolidSyslogError, SolidSyslogCreateWithNullConfigReportsError)
{
    installHandler();

    SolidSyslog_Create(nullptr);

    CALLED_FUNCTION(handler, ONCE);
    CHECK_EXPECTED_SEVERITY(SOLIDSYSLOG_SEVERITY_ERR);
    CHECK_EXPECTED_MESSAGE(SOLIDSYSLOG_ERROR_MSG_CREATE_NULL_CONFIG);
}
