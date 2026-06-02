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
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &source, 0U, 0);
}

TEST(SolidSyslogErrorEx, InstalledHandlerReceivesEventFieldsAndContext)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    ErrorHandlerFake_Install(&sentinel);

    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, &source, 7U, 42);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&source, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(7U, ErrorHandlerFake_LastCategory());
    LONGS_EQUAL(42, ErrorHandlerFake_LastDetail());
    POINTERS_EQUAL(&sentinel, ErrorHandlerFake_LastContext());
}

TEST(SolidSyslogErrorEx, SetErrorHandlerExWithNullHandlerRestoresDefault)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    ErrorHandlerFake_Install(&sentinel);

    SolidSyslog_SetErrorHandler(nullptr, &sentinel);
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &source, 0U, 0);

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}
