#include <stdint.h>
#include <string>

#include "CppUTest/TestHarness.h"

#include "SenderFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogError.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSdElement.h"
#include "SolidSyslogStructuredDataDefinition.h"
#include "StringFake.h"
#include "SyslogFieldParser.h"
#include "TestUtils.h"

using namespace CososoTesting;

struct SolidSyslog;
struct SolidSyslogSdElement;
struct SolidSyslogSender;

static const struct SolidSyslogErrorSource TEST_SOURCE = {"TestSource", nullptr};
static const int32_t TEST_DETAIL = 7;

/* The plain buffer loss most tests report: the test source and detail, with
 * no native code. */
static void RaiseBufferLoss()
{
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &TEST_SOURCE, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, TEST_DETAIL);
}

static void SdSpyFormat(struct SolidSyslogStructuredData* /* self */, struct SolidSyslogSdElement* element)
{
    SolidSyslogSdElement_Begin(element, "spy", 0U);
    SolidSyslogSdElement_End(element);
}

static struct SolidSyslogStructuredData sdSpy = {SdSpyFormat};

/* A logger with its own identity and one SD of its own, sending straight to
 * the sender fake: the report Service makes is the only record that can
 * reach it. */
// clang-format off
TEST_GROUP(SolidSyslogServiceEventReport)
{
    struct SolidSyslogSender* fakeSender = nullptr;
    struct SolidSyslog* solidSyslog = nullptr;
    struct SolidSyslogStructuredData* sds[1] = {&sdSpy};

    void setup() override
    {
        fakeSender = SenderFake_Create();
        StringFake_Reset();
        struct SolidSyslogConfig config = {};
        config.Buffer = SolidSyslogNullBuffer_Get();
        config.Sender = fakeSender;
        config.Store = SolidSyslogNullStore_Get();
        config.GetHostname = StringFake_GetHostname;
        config.GetAppName = StringFake_GetAppName;
        config.GetProcessId = StringFake_GetProcessId;
        config.Sd = sds;
        config.SdCount = 1U;
        solidSyslog = SolidSyslog_Create(&config);
    }

    void teardown() override
    {
        SolidSyslog_Destroy(solidSyslog);
        SenderFake_Destroy(fakeSender);
    }

    [[nodiscard]] std::string LastField(int field) const
    {
        return SyslogField(SenderFake_LastBufferAsString(fakeSender), field);
    }

    [[nodiscard]] std::string LastMsg() const
    {
        return SyslogMsg(SenderFake_LastBufferAsString(fakeSender));
    }
};

// clang-format on

TEST(SolidSyslogServiceEventReport, APendingBufferLossIsSentByTheNextService)
{
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
}

TEST(SolidSyslogServiceEventReport, TheReportIsAtFacilitySyslogAndTheEventsSeverity)
{
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, &TEST_SOURCE, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, TEST_DETAIL);

    SolidSyslog_Service(solidSyslog);

    STRCMP_EQUAL("<44>1", LastField(SYSLOG_FIELD_HEADER).c_str());
}

TEST(SolidSyslogServiceEventReport, TheReportCarriesTheLoggersOwnHostnameAppNameAndProcid)
{
    StringFake_SetHostname("TestHost");
    StringFake_SetAppName("TestApp");
    StringFake_SetProcessId("42");
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    STRCMP_EQUAL("TestHost", LastField(SYSLOG_FIELD_HOSTNAME).c_str());
    STRCMP_EQUAL("TestApp", LastField(SYSLOG_FIELD_APP_NAME).c_str());
    STRCMP_EQUAL("42", LastField(SYSLOG_FIELD_PROCID).c_str());
}

TEST(SolidSyslogServiceEventReport, TheReportsMsgIdIsTheCategoryName)
{
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    STRCMP_EQUAL("BUFFER_RECORDS_LOST", LastField(SYSLOG_FIELD_MSGID).c_str());
}

TEST(SolidSyslogServiceEventReport, TheReportsMsgSaysItIsASolidSyslogEvent)
{
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    STRCMP_EQUAL("SolidSyslog event", LastMsg().c_str());
}

TEST(SolidSyslogServiceEventReport, TheLibrarysElementFollowsTheLoggersOwnSd)
{
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS("[spy][solidsyslog@67478", LastField(SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslogServiceEventReport, TheElementNamesTheEventsSource)
{
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS(R"([solidsyslog@67478 source="TestSource")", LastField(SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslogServiceEventReport, TheElementNamesTheEventsCategoryAfterItsSource)
{
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS(R"(source="TestSource" category="BUFFER_RECORDS_LOST")", LastField(SYSLOG_FIELD_SDATA).c_str());
}

static const char* SevenIsNamed(int32_t detail)
{
    return (detail == 7) ? "SEVEN" : nullptr;
}

TEST(SolidSyslogServiceEventReport, TheElementNamesTheDetailAfterTheCategory)
{
    static const struct SolidSyslogErrorSource source = {"TestSource", SevenIsNamed};
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &source, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, TEST_DETAIL);

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS(R"(category="BUFFER_RECORDS_LOST" detail="SEVEN")", LastField(SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslogServiceEventReport, AnUnnamedDetailIsWrittenAsItsNumber)
{
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS(R"(detail="7")", LastField(SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslogServiceEventReport, ANegativeUnnamedDetailKeepsItsSign)
{
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &TEST_SOURCE, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, -3);

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS(R"(detail="-3")", LastField(SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslogServiceEventReport, TheElementCarriesTheNativeCodeAfterTheDetail)
{
    SolidSyslog_ErrorWithNativeCode(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &TEST_SOURCE,
        SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
        TEST_DETAIL,
        -5
    );

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS(R"(detail="7" native="-5")", LastField(SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslogServiceEventReport, AnEventWithNoNativeCodeHasNoNativeParam)
{
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);

    STRCMP_EQUAL(
        R"([spy][solidsyslog@67478 source="TestSource" category="BUFFER_RECORDS_LOST" detail="7"])",
        LastField(SYSLOG_FIELD_SDATA).c_str()
    );
}

TEST(SolidSyslogServiceEventReport, ANativeCodeOfZeroIsStillWritten)
{
    SolidSyslog_ErrorWithNativeCode(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &TEST_SOURCE,
        SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
        TEST_DETAIL,
        0
    );

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS(R"(native="0")", LastField(SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslogServiceEventReport, TheMostNegativeNativeCodeIsWrittenWhole)
{
    SolidSyslog_ErrorWithNativeCode(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &TEST_SOURCE,
        SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
        TEST_DETAIL,
        INT32_MIN
    );

    SolidSyslog_Service(solidSyslog);

    STRCMP_CONTAINS(R"(native="-2147483648")", LastField(SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslogServiceEventReport, AnEventWithNoSourceHasNoSourceParam)
{
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, nullptr, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, TEST_DETAIL);

    SolidSyslog_Service(solidSyslog);

    STRCMP_EQUAL(
        R"([spy][solidsyslog@67478 category="BUFFER_RECORDS_LOST" detail="7"])",
        LastField(SYSLOG_FIELD_SDATA).c_str()
    );
}

TEST(SolidSyslogServiceEventReport, ASourceWithNoNameHasNoSourceParam)
{
    static const struct SolidSyslogErrorSource source = {nullptr, nullptr};
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &source, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, TEST_DETAIL);

    SolidSyslog_Service(solidSyslog);

    STRCMP_EQUAL(
        R"([spy][solidsyslog@67478 category="BUFFER_RECORDS_LOST" detail="7"])",
        LastField(SYSLOG_FIELD_SDATA).c_str()
    );
}

TEST(SolidSyslogServiceEventReport, NothingIsSentWhenNothingIsPending)
{
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, NEVER);
}

TEST(SolidSyslogServiceEventReport, SeveralRaisesOfOneKindAreReportedOnce)
{
    RaiseBufferLoss();
    RaiseBufferLoss();

    SolidSyslog_Service(solidSyslog);
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
}

TEST(SolidSyslogServiceEventReport, TheFallbackLoggerLeavesTheEventForALoggerThatCanReportIt)
{
    struct SolidSyslog* fallback = SolidSyslog_Create(nullptr);
    RaiseBufferLoss();

    SolidSyslog_Service(fallback);
    SolidSyslog_Service(solidSyslog);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
}
