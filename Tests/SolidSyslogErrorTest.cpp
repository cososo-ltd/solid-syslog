#include "CppUTest/TestHarness.h"

#include "ErrorHandlerFake.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "TestUtils.h"

using namespace CososoTesting;

// clang-format off
TEST_GROUP(SolidSyslogErrorEx)
{
    int sentinel = 0;
};

// clang-format on

TEST(SolidSyslogErrorEx, ErrorExWithDefaultHandlerDoesNotCrash)
{
    static const struct SolidSyslogErrorSource source = {"test"};
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &source, 0U, 0);
}

TEST(SolidSyslogErrorEx, InstalledHandlerReceivesEventFieldsAndContext)
{
    static const struct SolidSyslogErrorSource source = {"test"};
    ErrorHandlerFake_Install(&sentinel);

    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, &source, 7U, 42);

    CHECK_ERROR_REPORTED_ONCE(SOLIDSYSLOG_SEVERITY_WARNING, &source, 7U, 42);
    POINTERS_EQUAL(&sentinel, ErrorHandlerFake_LastContext());
}

TEST(SolidSyslogErrorEx, SetErrorHandlerExWithNullHandlerRestoresDefault)
{
    static const struct SolidSyslogErrorSource source = {"test"};
    ErrorHandlerFake_Install(&sentinel);

    SolidSyslog_SetErrorHandler(nullptr, &sentinel);
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &source, 0U, 0);

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}
