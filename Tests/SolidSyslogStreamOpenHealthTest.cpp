#include <stdint.h>

#include "CppUTest/TestHarness.h"

#include "ErrorHandlerFake.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamOpenHealth.h"
#include "TestUtils.h"

using namespace CososoTesting;

namespace
{
const struct SolidSyslogErrorSource TestStreamSource = {"TestStream", nullptr};

constexpr uint16_t TEST_CATEGORY = 7;
constexpr int32_t TEST_DETAIL = 3;
constexpr int32_t OTHER_DETAIL = 4;
constexpr int32_t TEST_ERRNO = 111;
constexpr int32_t OTHER_ERRNO = 110;
} // namespace

// clang-format off
TEST_GROUP(SolidSyslogStreamOpenHealth)
{
    struct SolidSyslogStreamOpenHealth health = {};

    void setup() override
    {
        ErrorHandlerFake_Install(nullptr);
    }

    void teardown() override
    {
        SolidSyslog_SetErrorHandler(nullptr, nullptr);
    }

    void reportFailure(enum SolidSyslogSeverity severity, int32_t detail)
    {
        SolidSyslogStreamOpenHealth_ReportFailure(&health, &TestStreamSource, severity, TEST_CATEGORY, detail);
    }

    void reportFailureWithErrno(int32_t detail, int32_t nativeErrno)
    {
        SolidSyslogStreamOpenHealth_ReportFailureWithNativeCode(
            &health, &TestStreamSource, SOLIDSYSLOG_SEVERITY_WARNING, TEST_CATEGORY, detail, nativeErrno
        );
    }
};

// clang-format on

TEST(SolidSyslogStreamOpenHealth, FirstFailureIsReportedAsGiven)
{
    reportFailure(SOLIDSYSLOG_SEVERITY_WARNING, TEST_DETAIL);

    CHECK_ERROR_REPORTED_ONCE(SOLIDSYSLOG_SEVERITY_WARNING, &TestStreamSource, TEST_CATEGORY, TEST_DETAIL);
}

TEST(SolidSyslogStreamOpenHealth, RepeatOfTheSameDetailIsNotReportedAgain)
{
    reportFailure(SOLIDSYSLOG_SEVERITY_WARNING, TEST_DETAIL);
    reportFailure(SOLIDSYSLOG_SEVERITY_WARNING, TEST_DETAIL);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
}

TEST(SolidSyslogStreamOpenHealth, ADifferentDetailIsReportedWithItsOwnSeverity)
{
    reportFailure(SOLIDSYSLOG_SEVERITY_WARNING, TEST_DETAIL);
    reportFailure(SOLIDSYSLOG_SEVERITY_ERROR, OTHER_DETAIL);

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
    CHECK_ERROR_EVENT(SOLIDSYSLOG_SEVERITY_ERROR, &TestStreamSource, TEST_CATEGORY, OTHER_DETAIL);
}

TEST(SolidSyslogStreamOpenHealth, ASuccessfulOpenReArmsTheSameDetail)
{
    reportFailure(SOLIDSYSLOG_SEVERITY_WARNING, TEST_DETAIL);
    SolidSyslogStreamOpenHealth_Opened(&health);
    reportFailure(SOLIDSYSLOG_SEVERITY_WARNING, TEST_DETAIL);

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
}

TEST(SolidSyslogStreamOpenHealth, FirstFailureWithANativeCodeCarriesIt)
{
    reportFailureWithErrno(TEST_DETAIL, TEST_ERRNO);

    CHECK_ERROR_REPORTED_ONCE_WITH_NATIVE_CODE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &TestStreamSource,
        TEST_CATEGORY,
        TEST_DETAIL,
        TEST_ERRNO
    );
}

TEST(SolidSyslogStreamOpenHealth, ANewNativeCodeBehindTheSameDetailIsNotReportedAgain)
{
    reportFailureWithErrno(TEST_DETAIL, TEST_ERRNO);
    reportFailureWithErrno(TEST_DETAIL, OTHER_ERRNO);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
}
