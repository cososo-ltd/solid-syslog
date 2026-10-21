#include "CppUTest/TestHarness.h"

#include "ErrorHandlerFake.h"
#include "SolidSyslogDatagramErrors.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
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

    CHECK_ERROR_REPORTED_ONCE(SOLIDSYSLOG_SEVERITY_WARNING, &source, 7U, 42);
    POINTERS_EQUAL(&sentinel, ErrorHandlerFake_LastContext());
}

TEST(SolidSyslogErrorEx, AnEventFromErrorCarriesNoNativeCode)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    ErrorHandlerFake_Install(&sentinel);

    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, &source, 7U, 42);

    LONGS_EQUAL(0, ErrorHandlerFake_LastNativeCode());
}

TEST(SolidSyslogErrorEx, AnEventFromErrorWithNativeCodeCarriesIt)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    ErrorHandlerFake_Install(&sentinel);

    SolidSyslog_ErrorWithNativeCode(SOLIDSYSLOG_SEVERITY_WARNING, &source, 7U, 42, -13);

    CHECK_ERROR_REPORTED_ONCE(SOLIDSYSLOG_SEVERITY_WARNING, &source, 7U, 42);
    LONGS_EQUAL(-13, ErrorHandlerFake_LastNativeCode());
}

TEST(SolidSyslogErrorEx, SetErrorHandlerExWithNullHandlerRestoresDefault)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    ErrorHandlerFake_Install(&sentinel);

    SolidSyslog_SetErrorHandler(nullptr, &sentinel);
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &source, 0U, 0);

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogErrorEx, AnEventNamesItsDetailThroughItsSource)
{
    static const struct SolidSyslogErrorSource source = {"test", SolidSyslogDatagramErrors_Name};
    const struct SolidSyslogErrorEvent event =
        {SOLIDSYSLOG_SEVERITY_WARNING, 0U, &source, SOLIDSYSLOG_DATAGRAM_ERROR_NEXT_HOP_UNRESOLVED, 0};

    STRCMP_EQUAL("NEXT_HOP_UNRESOLVED", SolidSyslogErrorEvent_DetailName(&event));
}

TEST(SolidSyslogErrorEx, AnEventWithANativeCodeStillNamesItsDetail)
{
    static const struct SolidSyslogErrorSource source = {"test", SolidSyslogDatagramErrors_Name};
    const struct SolidSyslogErrorEvent event =
        {SOLIDSYSLOG_SEVERITY_WARNING, 0U, &source, SOLIDSYSLOG_DATAGRAM_ERROR_NEXT_HOP_UNRESOLVED, 2};

    STRCMP_EQUAL("NEXT_HOP_UNRESOLVED", SolidSyslogErrorEvent_DetailName(&event));
}

TEST(SolidSyslogErrorEx, AnEventWithoutASourceHasNoDetailName)
{
    const struct SolidSyslogErrorEvent event = {SOLIDSYSLOG_SEVERITY_WARNING, 0U, nullptr, 2, 0};

    POINTERS_EQUAL(nullptr, SolidSyslogErrorEvent_DetailName(&event));
}

TEST(SolidSyslogErrorEx, AnEventWhoseSourceNamesNothingHasNoDetailName)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    const struct SolidSyslogErrorEvent event = {SOLIDSYSLOG_SEVERITY_WARNING, 0U, &source, 2, 0};

    POINTERS_EQUAL(nullptr, SolidSyslogErrorEvent_DetailName(&event));
}
