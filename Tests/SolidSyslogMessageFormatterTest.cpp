#include <cstring>
#include <string>

#include "CppUTest/TestHarness.h"
#include "SolidSyslog.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogMessageFormatter.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTimestamp.h"
#include "SolidSyslogTunables.h"
#include "StringFake.h"
#include "SyslogFieldParser.h"

static struct SolidSyslogTimestamp stubTimestamp;

static void MessageFormatterTestClock(struct SolidSyslogTimestamp* timestamp)
{
    *timestamp = stubTimestamp;
}

#define OUTPUT() SolidSyslogFormatter_AsFormattedBuffer(formatter)

#define CHECK_PRIVAL(expected) \
    STRNCMP_EQUAL(expected, SyslogField(OUTPUT(), SYSLOG_FIELD_HEADER).c_str(), strlen(expected))
#define CHECK_TIMESTAMP(expected) STRCMP_EQUAL(expected, SyslogField(OUTPUT(), SYSLOG_FIELD_TIMESTAMP).c_str())
#define CHECK_HOSTNAME(expected) STRCMP_EQUAL(expected, SyslogField(OUTPUT(), SYSLOG_FIELD_HOSTNAME).c_str())
#define CHECK_APP_NAME(expected) STRCMP_EQUAL(expected, SyslogField(OUTPUT(), SYSLOG_FIELD_APP_NAME).c_str())
#define CHECK_PROCID(expected) STRCMP_EQUAL(expected, SyslogField(OUTPUT(), SYSLOG_FIELD_PROCID).c_str())
#define CHECK_MSGID(expected) STRCMP_EQUAL(expected, SyslogField(OUTPUT(), SYSLOG_FIELD_MSGID).c_str())

TEST_GROUP(SolidSyslogMessageFormatter)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_MESSAGE_SIZE)];
    struct SolidSyslogFormatter* formatter;
    struct SolidSyslogMessage message;
    struct SolidSyslogMessageFormatterContext context;

    void setup() override
    {
        formatter = SolidSyslogFormatter_Create(storage, SOLIDSYSLOG_MAX_MESSAGE_SIZE);
        StringFake_Reset();
        stubTimestamp = {};
        context = {
            MessageFormatterTestClock,
            StringFake_GetHostname,
            nullptr,
            StringFake_GetAppName,
            nullptr,
            StringFake_GetProcessId,
            nullptr,
            nullptr,
            0
        };
        message = {SOLIDSYSLOG_FACILITY_LOCAL0, SOLIDSYSLOG_SEVERITY_INFORMATIONAL, nullptr, nullptr};
    }

    void format()
    {
        SolidSyslogMessageFormatter_Format(formatter, &message, &context, nullptr, 0);
    }

    // Clears the output buffer between two format() calls without rebuilding
    // the context, so "callback invoked per call" tests exercise consecutive
    // formats on the same fixture (a cached callback result would be caught).
    void resetFormatter()
    {
        formatter = SolidSyslogFormatter_Create(storage, SOLIDSYSLOG_MAX_MESSAGE_SIZE);
    }
};

TEST(SolidSyslogMessageFormatter, PriValIs134)
{
    format();
    CHECK_PRIVAL("<134>");
}

TEST(SolidSyslogMessageFormatter, FacilityAppearsInPrival)
{
    message.Facility = SOLIDSYSLOG_FACILITY_NEWS;
    format();
    CHECK_PRIVAL("<62>");
}

TEST(SolidSyslogMessageFormatter, SeverityAppearsInPrival)
{
    message.Severity = SOLIDSYSLOG_SEVERITY_CRITICAL;
    format();
    CHECK_PRIVAL("<130>");
}

TEST(SolidSyslogMessageFormatter, LowestFacilityProducesCorrectPrival)
{
    message.Facility = SOLIDSYSLOG_FACILITY_KERN;
    format();
    CHECK_PRIVAL("<6>");
}

TEST(SolidSyslogMessageFormatter, HighestFacilityProducesCorrectPrival)
{
    message.Facility = SOLIDSYSLOG_FACILITY_LOCAL7;
    format();
    CHECK_PRIVAL("<190>");
}

TEST(SolidSyslogMessageFormatter, LowestSeverityProducesCorrectPrival)
{
    message.Severity = SOLIDSYSLOG_SEVERITY_EMERGENCY;
    format();
    CHECK_PRIVAL("<128>");
}

TEST(SolidSyslogMessageFormatter, HighestSeverityProducesCorrectPrival)
{
    message.Severity = SOLIDSYSLOG_SEVERITY_DEBUG;
    format();
    CHECK_PRIVAL("<135>");
}

TEST(SolidSyslogMessageFormatter, OutOfRangeFacilityProducesErrorPrival)
{
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange) -- intentionally testing out-of-range input
    message.Facility = (enum SolidSyslogFacility) 24;
    format();
    CHECK_PRIVAL("<43>");
}

TEST(SolidSyslogMessageFormatter, OutOfRangeSeverityProducesErrorPrival)
{
    enum SolidSyslogSeverity invalid = SOLIDSYSLOG_SEVERITY_DEBUG;
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange) -- intentionally testing out-of-range input
    invalid = static_cast<enum SolidSyslogSeverity>(static_cast<int>(invalid) + 1);
    message.Severity = invalid;
    format();
    CHECK_PRIVAL("<43>");
}

TEST(SolidSyslogMessageFormatter, HostnameFromGetHostnameAppearsInMessage)
{
    StringFake_SetHostname("MyHost");
    format();
    CHECK_HOSTNAME("MyHost");
}

TEST(SolidSyslogMessageFormatter, HostnameCallbackIsInvokedPerFormatCall)
{
    StringFake_SetHostname("FirstHost");
    format();
    CHECK_HOSTNAME("FirstHost");
    resetFormatter();
    StringFake_SetHostname("SecondHost");
    format();
    CHECK_HOSTNAME("SecondHost");
}

TEST(SolidSyslogMessageFormatter, HostnameAt255CharsIsAccepted)
{
    std::string longHostname(255, 'H');
    StringFake_SetHostname(longHostname.c_str());
    format();
    CHECK_HOSTNAME(longHostname.c_str());
}

TEST(SolidSyslogMessageFormatter, HostnameAt256CharsIsTruncatedTo255)
{
    std::string longHostname(256, 'H');
    StringFake_SetHostname(longHostname.c_str());
    format();
    std::string expected(255, 'H');
    CHECK_HOSTNAME(expected.c_str());
}

TEST(SolidSyslogMessageFormatter, HostnameNonPrintableByteIsSubstitutedWithQuestionMark)
{
    StringFake_SetHostname("a\x01"
                           "b");
    format();
    CHECK_HOSTNAME("a?b");
}

TEST(SolidSyslogMessageFormatter, EmptyHostnameProducesNilvalue)
{
    StringFake_SetHostname("");
    format();
    CHECK_HOSTNAME("-");
}

TEST(SolidSyslogMessageFormatter, AppNameFromGetAppNameAppearsInMessage)
{
    StringFake_SetAppName("MyApp");
    format();
    CHECK_APP_NAME("MyApp");
}

TEST(SolidSyslogMessageFormatter, AppNameCallbackIsInvokedPerFormatCall)
{
    StringFake_SetAppName("FirstApp");
    format();
    CHECK_APP_NAME("FirstApp");
    resetFormatter();
    StringFake_SetAppName("SecondApp");
    format();
    CHECK_APP_NAME("SecondApp");
}

TEST(SolidSyslogMessageFormatter, AppNameAt48CharsIsAccepted)
{
    std::string longAppName(48, 'A');
    StringFake_SetAppName(longAppName.c_str());
    format();
    CHECK_APP_NAME(longAppName.c_str());
}

TEST(SolidSyslogMessageFormatter, AppNameAt49CharsIsTruncatedTo48)
{
    std::string longAppName(49, 'A');
    StringFake_SetAppName(longAppName.c_str());
    format();
    std::string expected(48, 'A');
    CHECK_APP_NAME(expected.c_str());
}

TEST(SolidSyslogMessageFormatter, AppNameNonPrintableByteIsSubstitutedWithQuestionMark)
{
    StringFake_SetAppName("a\x7F"
                          "b");
    format();
    CHECK_APP_NAME("a?b");
}

TEST(SolidSyslogMessageFormatter, EmptyAppNameProducesNilvalue)
{
    StringFake_SetAppName("");
    format();
    CHECK_APP_NAME("-");
}

TEST(SolidSyslogMessageFormatter, ProcessIdFromGetProcessIdAppearsInMessage)
{
    StringFake_SetProcessId("9999");
    format();
    CHECK_PROCID("9999");
}

TEST(SolidSyslogMessageFormatter, ProcessIdCallbackIsInvokedPerFormatCall)
{
    StringFake_SetProcessId("1111");
    format();
    CHECK_PROCID("1111");
    resetFormatter();
    StringFake_SetProcessId("2222");
    format();
    CHECK_PROCID("2222");
}

TEST(SolidSyslogMessageFormatter, ProcessIdAt128CharsIsAccepted)
{
    std::string longProcessId(128, 'P');
    StringFake_SetProcessId(longProcessId.c_str());
    format();
    CHECK_PROCID(longProcessId.c_str());
}

TEST(SolidSyslogMessageFormatter, ProcessIdAt129CharsIsTruncatedTo128)
{
    std::string longProcessId(129, 'P');
    StringFake_SetProcessId(longProcessId.c_str());
    format();
    std::string expected(128, 'P');
    CHECK_PROCID(expected.c_str());
}

TEST(SolidSyslogMessageFormatter, ProcessIdNonPrintableByteIsSubstitutedWithQuestionMark)
{
    StringFake_SetProcessId("a\xC3"
                            "b");
    format();
    CHECK_PROCID("a?b");
}

TEST(SolidSyslogMessageFormatter, EmptyProcessIdProducesNilvalue)
{
    StringFake_SetProcessId("");
    format();
    CHECK_PROCID("-");
}

TEST(SolidSyslogMessageFormatter, NullMessageIdProducesNilvalue)
{
    format();
    CHECK_MSGID("-");
}

TEST(SolidSyslogMessageFormatter, MessageIdAppearsInMessage)
{
    message.MessageId = "ID47";
    format();
    CHECK_MSGID("ID47");
}

TEST(SolidSyslogMessageFormatter, MessageIdIsNotHardCoded)
{
    message.MessageId = "54";
    format();
    CHECK_MSGID("54");
}

TEST(SolidSyslogMessageFormatter, EmptyMessageIdProducesNilvalue)
{
    message.MessageId = "";
    format();
    CHECK_MSGID("-");
}

TEST(SolidSyslogMessageFormatter, MessageIdAt32CharsIsAccepted)
{
    std::string maxMsgId(32, 'M');
    message.MessageId = maxMsgId.c_str();
    format();
    CHECK_MSGID(maxMsgId.c_str());
}

TEST(SolidSyslogMessageFormatter, MessageIdAt33CharsIsTruncatedTo32)
{
    std::string longMsgId(33, 'M');
    message.MessageId = longMsgId.c_str();
    format();
    std::string expected(32, 'M');
    CHECK_MSGID(expected.c_str());
}

TEST(SolidSyslogMessageFormatter, MessageIdNonPrintableByteIsSubstitutedWithQuestionMark)
{
    message.MessageId = "a b";
    format();
    CHECK_MSGID("a?b");
}

TEST(SolidSyslogMessageFormatter, AllFieldsAtMaxLengthProducesValidMessage)
{
    std::string maxHostname(255, 'H');
    std::string maxAppName(48, 'A');
    std::string maxProcessId(128, 'P');
    StringFake_SetHostname(maxHostname.c_str());
    StringFake_SetAppName(maxAppName.c_str());
    StringFake_SetProcessId(maxProcessId.c_str());
    stubTimestamp = {9999, 12, 31, 23, 59, 59, 999999, 840};
    message.Facility = SOLIDSYSLOG_FACILITY_LOCAL7;
    message.Severity = SOLIDSYSLOG_SEVERITY_DEBUG;
    format();
    CHECK_PRIVAL("<191>");
    CHECK_TIMESTAMP("9999-12-31T23:59:59.999999+14:00");
    CHECK_HOSTNAME(maxHostname.c_str());
    CHECK_APP_NAME(maxAppName.c_str());
    CHECK_PROCID(maxProcessId.c_str());
}

// RFC 5424 §6: SD then SP (outer framing) then BOM (first byte of MSG-UTF8).
TEST(SolidSyslogMessageFormatter, MsgFieldIsSpaceThenBomThenBody)
{
    message.Msg = "hello";
    format();
    STRCMP_EQUAL("<134>1 - - - - - - \xEF\xBB\xBFhello", OUTPUT());
}

TEST(SolidSyslogMessageFormatter, BomOnlyMessageProducesEmptyMsgField)
{
    message.Msg = "\xEF\xBB\xBF";
    format();
    STRCMP_EQUAL("<134>1 - - - - - -", OUTPUT());
}
