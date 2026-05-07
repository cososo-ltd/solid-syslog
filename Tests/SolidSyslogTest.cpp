#include <stdint.h>
#include <cstring>
#include <string>

#include "SolidSyslog.h"
#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogMetaSd.h"
#include "TestAtomicOps.h"
#include "SolidSyslogTimeQualitySd.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogStructuredDataDefinition.h"
#include "BufferFake.h"
#include "StoreFake.h"
#include "SenderFake.h"
#include "StringFake.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogTimeQuality.h"
#include "SolidSyslogTimestamp.h"
#include "CppUTest/TestHarness.h"

class TEST_SolidSyslogTimestamp_Day0ProducesNilvalue_Test;
class TEST_SolidSyslogTimestamp_Day1FormatsAs01_Test;
class TEST_SolidSyslogTimestamp_Day31FormatsAs31_Test;
class TEST_SolidSyslogTimestamp_Day32ProducesNilvalue_Test;
class TEST_SolidSyslogTimestamp_DayFormatsAsTwoDigitZeroPadded_Test;
class TEST_SolidSyslogTimestamp_Hour0FormatsAs00_Test;
class TEST_SolidSyslogTimestamp_Hour23FormatsAs23_Test;
class TEST_SolidSyslogTimestamp_Hour24ProducesNilvalue_Test;
class TEST_SolidSyslogTimestamp_HourFormatsAsTwoDigitZeroPadded_Test;
class TEST_SolidSyslogTimestamp_Microsecond0FormatsAs000000_Test;
class TEST_SolidSyslogTimestamp_Microsecond1000000ProducesNilvalue_Test;
class TEST_SolidSyslogTimestamp_Microsecond999999FormatsAs999999_Test;
class TEST_SolidSyslogTimestamp_MicrosecondFormatsAsSixDigitZeroPadded_Test;
class TEST_SolidSyslogTimestamp_Minute0FormatsAs00_Test;
class TEST_SolidSyslogTimestamp_Minute59FormatsAs59_Test;
class TEST_SolidSyslogTimestamp_Minute60ProducesNilvalue_Test;
class TEST_SolidSyslogTimestamp_MinuteFormatsAsTwoDigitZeroPadded_Test;
class TEST_SolidSyslogTimestamp_Month0ProducesNilvalue_Test;
class TEST_SolidSyslogTimestamp_Month12FormatsAs12_Test;
class TEST_SolidSyslogTimestamp_Month13ProducesNilvalue_Test;
class TEST_SolidSyslogTimestamp_Month1FormatsAs01_Test;
class TEST_SolidSyslogTimestamp_MonthFormatsAsTwoDigitZeroPadded_Test;
class TEST_SolidSyslogTimestamp_NegativeOffsetFormatsAsMinusHHMM_Test;
class TEST_SolidSyslogTimestamp_NullClockProducesNilvalue_Test;
class TEST_SolidSyslogTimestamp_PositiveOffsetFormatsAsPlusHHMM_Test;
class TEST_SolidSyslogTimestamp_Second0FormatsAs00_Test;
class TEST_SolidSyslogTimestamp_Second59FormatsAs59_Test;
class TEST_SolidSyslogTimestamp_Second60ProducesNilvalue_Test;
class TEST_SolidSyslogTimestamp_SecondFormatsAsTwoDigitZeroPadded_Test;
class TEST_SolidSyslogTimestamp_TimestampAppearsInCorrectMessageFieldPosition_Test;
class TEST_SolidSyslogTimestamp_UtcOffsetMinus720FormatsAsMinus1200_Test;
class TEST_SolidSyslogTimestamp_UtcOffsetMinus721ProducesNilvalue_Test;
class TEST_SolidSyslogTimestamp_UtcOffsetPlus840FormatsAsPlus1400_Test;
class TEST_SolidSyslogTimestamp_UtcOffsetPlus841ProducesNilvalue_Test;
class TEST_SolidSyslogTimestamp_Year9999FormatsAs9999_Test;
class TEST_SolidSyslogTimestamp_YearFormatsAsFourDigitZeroPadded_Test;
class TEST_SolidSyslogTimestamp_YearZeroFormatsAs0000_Test;
class TEST_SolidSyslogTimestamp_ZeroOffsetFormatsAsZ_Test;
class TEST_SolidSyslog_AllFieldsAtMaxLengthProducesValidMessage_Test;
class TEST_SolidSyslog_AppNameAt48CharsIsAccepted_Test;
class TEST_SolidSyslog_AppNameAt49CharsIsTruncatedTo48_Test;
class TEST_SolidSyslog_AppNameCallbackIsInvokedPerLogCall_Test;
class TEST_SolidSyslog_AppNameFromGetAppNameAppearsInMessage_Test;
class TEST_SolidSyslog_AppNameNonPrintableByteIsSubstitutedWithQuestionMark_Test;
class TEST_SolidSyslog_EmptyAppNameProducesNilvalue_Test;
class TEST_SolidSyslog_EmptyHostnameProducesNilvalue_Test;
class TEST_SolidSyslog_EmptyMessageIdProducesNilvalue_Test;
class TEST_SolidSyslog_EmptyProcessIdProducesNilvalue_Test;
class TEST_SolidSyslog_FacilityAppearsInPrival_Test;
class TEST_SolidSyslog_HighestFacilityProducesCorrectPrival_Test;
class TEST_SolidSyslog_HighestSeverityProducesCorrectPrival_Test;
class TEST_SolidSyslog_HostnameAt255CharsIsAccepted_Test;
class TEST_SolidSyslog_HostnameAt256CharsIsTruncatedTo255_Test;
class TEST_SolidSyslog_HostnameCallbackIsInvokedPerLogCall_Test;
class TEST_SolidSyslog_HostnameFromGetHostnameAppearsInMessage_Test;
class TEST_SolidSyslog_HostnameNonPrintableByteIsSubstitutedWithQuestionMark_Test;
class TEST_SolidSyslog_LogAfterDestroyAndRecreateWithNullFunctionsProducesNilvalues_Test;
class TEST_SolidSyslog_LowestFacilityProducesCorrectPrival_Test;
class TEST_SolidSyslog_LowestSeverityProducesCorrectPrival_Test;
class TEST_SolidSyslog_MessageIdAppearsInMessage_Test;
class TEST_SolidSyslog_MessageIdAt32CharsIsAccepted_Test;
class TEST_SolidSyslog_MessageIdAt33CharsIsTruncatedTo32_Test;
class TEST_SolidSyslog_MessageIdIsNotHardCoded_Test;
class TEST_SolidSyslog_MessageIdNonPrintableByteIsSubstitutedWithQuestionMark_Test;
class TEST_SolidSyslog_NullGetAppNameProducesNilvalue_Test;
class TEST_SolidSyslog_NullGetHostnameProducesNilvalue_Test;
class TEST_SolidSyslog_NullGetProcessIdProducesNilvalue_Test;
class TEST_SolidSyslog_NullMessageIdProducesNilvalue_Test;
class TEST_SolidSyslog_OutOfRangeFacilityProducesErrorPrival_Test;
class TEST_SolidSyslog_OutOfRangeSeverityProducesErrorPrival_Test;
class TEST_SolidSyslog_PriValIs134_Test;
class TEST_SolidSyslog_ProcessIdAt128CharsIsAccepted_Test;
class TEST_SolidSyslog_ProcessIdAt129CharsIsTruncatedTo128_Test;
class TEST_SolidSyslog_ProcessIdCallbackIsInvokedPerLogCall_Test;
class TEST_SolidSyslog_ProcessIdFromGetProcessIdAppearsInMessage_Test;
class TEST_SolidSyslog_ProcessIdNonPrintableByteIsSubstitutedWithQuestionMark_Test;
class TEST_SolidSyslog_SeverityAppearsInPrival_Test;
struct SolidSyslogAtomicCounter;
struct SolidSyslogBuffer;
struct SolidSyslogStore;

// clang-format off
static const char * const TEST_PRIVAL    = "<134>";
static const char * const TEST_MSGID     = "54";
static const char * const TEST_SDATA     = "-";
static const char * const TEST_MSG       = "hello world";

static const int SYSLOG_FIELD_HEADER    = 0;
static const int SYSLOG_FIELD_TIMESTAMP = 1;
static const int SYSLOG_FIELD_HOSTNAME  = 2;
static const int SYSLOG_FIELD_APP_NAME  = 3;
static const int SYSLOG_FIELD_PROCID    = 4;
static const int SYSLOG_FIELD_MSGID     = 5;
static const int SYSLOG_FIELD_SDATA     = 6;
// clang-format on

// clang-format off
static const int TIMESTAMP_YEAR_OFFSET        = 0;
static const int TIMESTAMP_YEAR_LENGTH        = 4;
static const int TIMESTAMP_MONTH_OFFSET       = 5;
static const int TIMESTAMP_MONTH_LENGTH       = 2;
static const int TIMESTAMP_DAY_OFFSET         = 8;
static const int TIMESTAMP_DAY_LENGTH         = 2;
static const int TIMESTAMP_HOUR_OFFSET        = 11;
static const int TIMESTAMP_HOUR_LENGTH        = 2;
static const int TIMESTAMP_MINUTE_OFFSET      = 14;
static const int TIMESTAMP_MINUTE_LENGTH      = 2;
static const int TIMESTAMP_SECOND_OFFSET      = 17;
static const int TIMESTAMP_SECOND_LENGTH      = 2;
static const int TIMESTAMP_DATE_SEPARATOR_1          = 4;
static const int TIMESTAMP_DATE_SEPARATOR_2          = 7;
static const int TIMESTAMP_DATE_TIME_SEPARATOR       = 10;
static const int TIMESTAMP_TIME_SEPARATOR_1          = 13;
static const int TIMESTAMP_TIME_SEPARATOR_2          = 16;
static const int TIMESTAMP_MICROSECOND_OFFSET  = 19;
static const int TIMESTAMP_MICROSECOND_LENGTH  = 7;
static const int TIMESTAMP_OFFSET_OFFSET       = 26;
// clang-format on

// NOLINTBEGIN(cppcoreguidelines-macro-usage) -- macros preserve __FILE__/__LINE__ in test failure output
#define CHECK_PRIVAL(expected) STRNCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_HEADER).c_str(), strlen(expected))

#define CHECK_TIMESTAMP_YEAR(expected) \
    STRNCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_TIMESTAMP).c_str() + TIMESTAMP_YEAR_OFFSET, TIMESTAMP_YEAR_LENGTH)

#define CHECK_TIMESTAMP_MONTH(expected) \
    STRNCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_TIMESTAMP).c_str() + TIMESTAMP_MONTH_OFFSET, TIMESTAMP_MONTH_LENGTH)

#define CHECK_TIMESTAMP_DAY(expected) \
    STRNCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_TIMESTAMP).c_str() + TIMESTAMP_DAY_OFFSET, TIMESTAMP_DAY_LENGTH)

#define CHECK_TIMESTAMP_HOUR(expected) \
    STRNCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_TIMESTAMP).c_str() + TIMESTAMP_HOUR_OFFSET, TIMESTAMP_HOUR_LENGTH)

#define CHECK_TIMESTAMP_MINUTE(expected) \
    STRNCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_TIMESTAMP).c_str() + TIMESTAMP_MINUTE_OFFSET, TIMESTAMP_MINUTE_LENGTH)

#define CHECK_TIMESTAMP_SECOND(expected) \
    STRNCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_TIMESTAMP).c_str() + TIMESTAMP_SECOND_OFFSET, TIMESTAMP_SECOND_LENGTH)

#define CHECK_TIMESTAMP_MICROSECOND(expected) \
    STRNCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_TIMESTAMP).c_str() + TIMESTAMP_MICROSECOND_OFFSET, TIMESTAMP_MICROSECOND_LENGTH)

#define CHECK_TIMESTAMP(expected) STRCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_TIMESTAMP).c_str())

#define CHECK_TIMESTAMP_OFFSET(expected) STRCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_TIMESTAMP).substr(TIMESTAMP_OFFSET_OFFSET).c_str())

#define CHECK_TIMESTAMP_IS_NILVALUE() STRCMP_EQUAL("-", SyslogField(lastMessage(), SYSLOG_FIELD_TIMESTAMP).c_str())

#define CHECK_HOSTNAME(expected) STRCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_HOSTNAME).c_str())

#define CHECK_APP_NAME(expected) STRCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_APP_NAME).c_str())

#define CHECK_PROCID(expected) STRCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_PROCID).c_str())

#define CHECK_MSGID(expected) STRCMP_EQUAL(expected, SyslogField(lastMessage(), SYSLOG_FIELD_MSGID).c_str())

// NOLINTEND(cppcoreguidelines-macro-usage)

static const char SD_SPY_TEXT[]  = "[spy]";
static const char SD_SPY2_TEXT[] = "[spy2]";

static void SdSpyFormat(struct SolidSyslogStructuredData* /* self */, struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_BoundedString(formatter, SD_SPY_TEXT, sizeof(SD_SPY_TEXT) - 1);
}

static struct SolidSyslogStructuredData sdSpy = {SdSpyFormat};

static void SdSpyFormat2(struct SolidSyslogStructuredData* /* self */, struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_BoundedString(formatter, SD_SPY2_TEXT, sizeof(SD_SPY2_TEXT) - 1);
}

static struct SolidSyslogStructuredData sdSpy2 = {SdSpyFormat2};

static void SdFailFormat(struct SolidSyslogStructuredData* /* self */, struct SolidSyslogFormatter* /* formatter */)
{
}

static struct SolidSyslogStructuredData sdFail = {SdFailFormat};

static void IntegrationGetTimeQuality(struct SolidSyslogTimeQuality* timeQuality)
{
    *timeQuality = {true, true, SOLIDSYSLOG_SYNC_ACCURACY_OMIT};
}

static std::string::size_type SkipSdata(const std::string& s, std::string::size_type pos)
{
    if (pos < s.size() && s[pos] == '[')
    {
        while (pos < s.size() && s[pos] == '[')
        {
            pos = s.find(']', pos);
            if (pos == std::string::npos)
            {
                return std::string::npos;
            }
            pos++;
        }
        return pos;
    }
    auto end = s.find(' ', pos);
    return end == std::string::npos ? s.size() : end;
}

static std::string::size_type FindFieldStart(const std::string& s, int n)
{
    std::string::size_type pos = 0;
    for (int i = 0; i < n; i++)
    {
        if (i == SYSLOG_FIELD_SDATA)
        {
            pos = SkipSdata(s, pos);
        }
        else
        {
            pos = s.find(' ', pos);
        }
        if (pos == std::string::npos)
        {
            return std::string::npos;
        }
        if (s[pos] == ' ')
        {
            pos++;
        }
    }
    return pos;
}

static std::string SyslogField(const char* buffer, int n)
{
    std::string            s(buffer);
    std::string::size_type pos = FindFieldStart(s, n);
    if (pos == std::string::npos)
    {
        return {};
    }

    std::string::size_type end = (n == SYSLOG_FIELD_SDATA) ? SkipSdata(s, pos) : s.find(' ', pos);
    return s.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
}

static const char UTF8_BOM[]      = "\xEF\xBB\xBF";
static const auto UTF8_BOM_LENGTH = sizeof(UTF8_BOM) - 1;

static std::string::size_type SyslogMsgStart(const std::string& s)
{
    std::string::size_type pos = FindFieldStart(s, SYSLOG_FIELD_SDATA);
    if (pos == std::string::npos)
    {
        return std::string::npos;
    }
    pos = SkipSdata(s, pos);
    if (pos == std::string::npos || pos >= s.size())
    {
        return std::string::npos;
    }
    if (s[pos] == ' ')
    {
        pos++;
    }
    return pos;
}

static bool SyslogMsgHasBom(const char* buffer)
{
    std::string            s(buffer);
    std::string::size_type pos = SyslogMsgStart(s);
    return (pos != std::string::npos) && (s.compare(pos, UTF8_BOM_LENGTH, UTF8_BOM) == 0);
}

static std::string SyslogMsg(const char* buffer)
{
    std::string            s(buffer);
    std::string::size_type pos = SyslogMsgStart(s);
    if (pos == std::string::npos)
    {
        return {};
    }
    if (s.compare(pos, UTF8_BOM_LENGTH, UTF8_BOM) == 0)
    {
        pos += UTF8_BOM_LENGTH;
    }
    return s.substr(pos);
}

// clang-format off
TEST_GROUP(SolidSyslog)
{
    SolidSyslogConfig config;
    SolidSyslogMessage message;
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    SolidSyslogBuffer *buffer;
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    SolidSyslogStore  *store;
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    struct SolidSyslogSender *fakeSender;

    void setup() override
    {
        fakeSender = SenderFake_Create();
        StringFake_Reset();
        buffer = SolidSyslogNullBuffer_Create(fakeSender);
        store  = SolidSyslogNullStore_Create();
        config = {buffer, nullptr, nullptr, StringFake_GetHostname, StringFake_GetAppName, StringFake_GetProcessId, store, nullptr, 0};
        SolidSyslog_Create(&config);
        // cppcheck-suppress unreadVariable -- read via Log() through &message; cppcheck does not model CppUTest macros
        message = {SOLIDSYSLOG_FACILITY_LOCAL0, SOLIDSYSLOG_SEVERITY_INFO, nullptr, nullptr};
    }

    void teardown() override
    {
        SolidSyslog_Destroy();
        SolidSyslogNullStore_Destroy();
        SolidSyslogNullBuffer_Destroy();
        SenderFake_Destroy(fakeSender);
    }

    void Log() const
    {
        SolidSyslog_Log(&message);
    }

    [[nodiscard]] const char *lastMessage() const
    {
        return SenderFake_LastBufferAsString(fakeSender);
    }
};

// clang-format on

TEST(SolidSyslog, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslog, NoMessagesAreSentWhenLogIsNotCalled)
{
    LONGS_EQUAL(0, SenderFake_SendCount(fakeSender));
}

TEST(SolidSyslog, SingleLogCallResultsInOneSend)
{
    Log();
    LONGS_EQUAL(1, SenderFake_SendCount(fakeSender));
}

TEST(SolidSyslog, PriValIs134)
{
    Log();
    CHECK_PRIVAL(TEST_PRIVAL);
}

TEST(SolidSyslog, FacilityAppearsInPrival)
{
    message.facility = SOLIDSYSLOG_FACILITY_NEWS;
    Log();
    CHECK_PRIVAL("<62>");
}

TEST(SolidSyslog, SeverityAppearsInPrival)
{
    message.severity = SOLIDSYSLOG_SEVERITY_CRIT;
    Log();
    CHECK_PRIVAL("<130>");
}

TEST(SolidSyslog, LowestFacilityProducesCorrectPrival)
{
    message.facility = SOLIDSYSLOG_FACILITY_KERN;
    Log();
    CHECK_PRIVAL("<6>");
}

TEST(SolidSyslog, HighestFacilityProducesCorrectPrival)
{
    message.facility = SOLIDSYSLOG_FACILITY_LOCAL7;
    Log();
    CHECK_PRIVAL("<190>");
}

TEST(SolidSyslog, LowestSeverityProducesCorrectPrival)
{
    message.severity = SOLIDSYSLOG_SEVERITY_EMERG;
    Log();
    CHECK_PRIVAL("<128>");
}

TEST(SolidSyslog, HighestSeverityProducesCorrectPrival)
{
    message.severity = SOLIDSYSLOG_SEVERITY_DEBUG;
    Log();
    CHECK_PRIVAL("<135>");
}

TEST(SolidSyslog, OutOfRangeFacilityProducesErrorPrival)
{
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange) -- intentionally testing out-of-range input
    message.facility = (enum SolidSyslog_Facility) 24;
    Log();
    CHECK_PRIVAL("<43>");
}

TEST(SolidSyslog, OutOfRangeSeverityProducesErrorPrival)
{
    enum SolidSyslog_Severity invalid = SOLIDSYSLOG_SEVERITY_DEBUG;
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange) -- intentionally testing out-of-range input
    invalid          = static_cast<enum SolidSyslog_Severity>(static_cast<int>(invalid) + 1);
    message.severity = invalid;
    Log();
    CHECK_PRIVAL("<43>");
}

TEST(SolidSyslog, VersionIs1)
{
    Log();
    std::string header         = SyslogField(lastMessage(), SYSLOG_FIELD_HEADER);
    auto        closingBracket = header.find('>');
    BYTES_EQUAL('1', header.at(closingBracket + 1));
}

TEST(SolidSyslog, NullGetHostnameProducesNilvalue)
{
    config.getHostname = nullptr;
    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    Log();
    CHECK_HOSTNAME("-");
}

TEST(SolidSyslog, HostnameFromGetHostnameAppearsInMessage)
{
    StringFake_SetHostname("MyHost");
    Log();
    CHECK_HOSTNAME("MyHost");
}

TEST(SolidSyslog, HostnameCallbackIsInvokedPerLogCall)
{
    StringFake_SetHostname("FirstHost");
    Log();
    CHECK_HOSTNAME("FirstHost");
    StringFake_SetHostname("SecondHost");
    Log();
    CHECK_HOSTNAME("SecondHost");
}

TEST(SolidSyslog, NullGetAppNameProducesNilvalue)
{
    config.getAppName = nullptr;
    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    Log();
    CHECK_APP_NAME("-");
}

TEST(SolidSyslog, AppNameFromGetAppNameAppearsInMessage)
{
    StringFake_SetAppName("MyApp");
    Log();
    CHECK_APP_NAME("MyApp");
}

TEST(SolidSyslog, AppNameCallbackIsInvokedPerLogCall)
{
    StringFake_SetAppName("FirstApp");
    Log();
    CHECK_APP_NAME("FirstApp");
    StringFake_SetAppName("SecondApp");
    Log();
    CHECK_APP_NAME("SecondApp");
}

TEST(SolidSyslog, NullGetProcessIdProducesNilvalue)
{
    config.getProcessId = nullptr;
    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    Log();
    CHECK_PROCID("-");
}

TEST(SolidSyslog, ProcessIdFromGetProcessIdAppearsInMessage)
{
    StringFake_SetProcessId("9999");
    Log();
    CHECK_PROCID("9999");
}

TEST(SolidSyslog, ProcessIdCallbackIsInvokedPerLogCall)
{
    StringFake_SetProcessId("1111");
    Log();
    CHECK_PROCID("1111");
    StringFake_SetProcessId("2222");
    Log();
    CHECK_PROCID("2222");
}

TEST(SolidSyslog, NullMessageIdProducesNilvalue)
{
    Log();
    CHECK_MSGID("-");
}

TEST(SolidSyslog, MessageIdAppearsInMessage)
{
    message.messageId = "ID47";
    Log();
    CHECK_MSGID("ID47");
}

TEST(SolidSyslog, MessageIdIsNotHardCoded)
{
    message.messageId = TEST_MSGID;
    Log();
    CHECK_MSGID(TEST_MSGID);
}

TEST(SolidSyslog, EmptyMessageIdProducesNilvalue)
{
    message.messageId = "";
    Log();
    CHECK_MSGID("-");
}

TEST(SolidSyslog, MessageIdAt32CharsIsAccepted)
{
    std::string maxMsgId(32, 'M');
    message.messageId = maxMsgId.c_str();
    Log();
    CHECK_MSGID(maxMsgId.c_str());
}

TEST(SolidSyslog, MessageIdAt33CharsIsTruncatedTo32)
{
    std::string longMsgId(33, 'M');
    message.messageId = longMsgId.c_str();
    Log();
    std::string expected(32, 'M');
    CHECK_MSGID(expected.c_str());
}

TEST(SolidSyslog, MessageIdNonPrintableByteIsSubstitutedWithQuestionMark)
{
    message.messageId = "a b";
    Log();
    CHECK_MSGID("a?b");
}

TEST(SolidSyslog, StructuredDataIsNilValue)
{
    Log();
    STRCMP_EQUAL(TEST_SDATA, SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslog, InjectedSdObjectFormatIsCalledDuringLog)
{
    SolidSyslogStructuredData* sdList[] = {&sdSpy};
    config.sd                           = sdList;
    config.sdCount                      = 1;
    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    Log();
    STRCMP_EQUAL("[spy]", SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslog, MetaSdProducesSequenceIdInStructuredData)
{
    SolidSyslogAtomicCounter* counter = SolidSyslogAtomicCounter_Create(TestAtomicOps_Create());
    SolidSyslogMetaSdConfig   metaConfig{};
    metaConfig.counter                  = counter;
    SolidSyslogStructuredData* metaSd   = SolidSyslogMetaSd_Create(&metaConfig);
    SolidSyslogStructuredData* sdList[] = {metaSd};
    config.sd                           = sdList;
    config.sdCount                      = 1;
    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    Log();
    STRCMP_EQUAL("[meta sequenceId=\"1\"]", SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
    SolidSyslogMetaSd_Destroy();
    SolidSyslogAtomicCounter_Destroy();
    TestAtomicOps_Destroy();
}

TEST(SolidSyslog, MetaSdSequenceIdIncrementsAcrossLogCalls)
{
    SolidSyslogAtomicCounter* counter = SolidSyslogAtomicCounter_Create(TestAtomicOps_Create());
    SolidSyslogMetaSdConfig   metaConfig{};
    metaConfig.counter                  = counter;
    SolidSyslogStructuredData* metaSd   = SolidSyslogMetaSd_Create(&metaConfig);
    SolidSyslogStructuredData* sdList[] = {metaSd};
    config.sd                           = sdList;
    config.sdCount                      = 1;
    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    Log();
    Log();
    STRCMP_EQUAL("[meta sequenceId=\"2\"]", SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
    SolidSyslogMetaSd_Destroy();
    SolidSyslogAtomicCounter_Destroy();
    TestAtomicOps_Destroy();
}

TEST(SolidSyslog, MsgFieldPreservedWithMetaSd)
{
    SolidSyslogAtomicCounter* counter = SolidSyslogAtomicCounter_Create(TestAtomicOps_Create());
    SolidSyslogMetaSdConfig   metaConfig{};
    metaConfig.counter                  = counter;
    SolidSyslogStructuredData* metaSd   = SolidSyslogMetaSd_Create(&metaConfig);
    SolidSyslogStructuredData* sdList[] = {metaSd};
    config.sd                           = sdList;
    config.sdCount                      = 1;
    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    message.msg = "hello world";
    Log();
    STRCMP_EQUAL("hello world", SyslogMsg(lastMessage()).c_str());
    SolidSyslogMetaSd_Destroy();
    SolidSyslogAtomicCounter_Destroy();
    TestAtomicOps_Destroy();
}

TEST(SolidSyslog, MultipleSdItemsAreConcatenated)
{
    SolidSyslogStructuredData* sdList[] = {&sdSpy, &sdSpy2};
    config.sd                           = sdList;
    config.sdCount                      = 2;
    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    Log();
    STRCMP_EQUAL("[spy][spy2]", SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslog, SingleSdReturningZeroBytesProducesNilvalue)
{
    SolidSyslogStructuredData* sdList[] = {&sdFail};
    config.sd                           = sdList;
    config.sdCount                      = 1;
    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    Log();
    STRCMP_EQUAL("-", SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslog, FailingSdIsSkippedWhenOtherSdSucceeds)
{
    SolidSyslogStructuredData* sdList[] = {&sdFail, &sdSpy};
    config.sd                           = sdList;
    config.sdCount                      = 2;
    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    Log();
    STRCMP_EQUAL("[spy]", SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslog, AllSdFailingProducesNilvalue)
{
    SolidSyslogStructuredData* sdList[] = {&sdFail, &sdFail};
    config.sd                           = sdList;
    config.sdCount                      = 2;
    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    Log();
    STRCMP_EQUAL("-", SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
}

TEST(SolidSyslog, MetaSdAndTimeQualitySdCoexistInSdArray)
{
    SolidSyslogAtomicCounter* counter = SolidSyslogAtomicCounter_Create(TestAtomicOps_Create());
    SolidSyslogMetaSdConfig   metaConfig{};
    metaConfig.counter                     = counter;
    SolidSyslogStructuredData* metaSd      = SolidSyslogMetaSd_Create(&metaConfig);
    SolidSyslogStructuredData* timeQuality = SolidSyslogTimeQualitySd_Create(IntegrationGetTimeQuality);
    SolidSyslogStructuredData* sdList[]    = {metaSd, timeQuality};
    config.sd                              = sdList;
    config.sdCount                         = 2;
    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    Log();
    STRCMP_EQUAL("[meta sequenceId=\"1\"][timeQuality tzKnown=\"1\" isSynced=\"1\"]", SyslogField(lastMessage(), SYSLOG_FIELD_SDATA).c_str());
    SolidSyslogTimeQualitySd_Destroy();
    SolidSyslogMetaSd_Destroy();
    SolidSyslogAtomicCounter_Destroy();
    TestAtomicOps_Destroy();
}

TEST(SolidSyslog, NullMessageOmitsMsgField)
{
    Log();
    CHECK_FALSE(SyslogMsgHasBom(lastMessage()));
    STRCMP_EQUAL("", SyslogMsg(lastMessage()).c_str());
}

TEST(SolidSyslog, MessageBodyAppearsInMessage)
{
    message.msg = "system started";
    Log();
    STRCMP_EQUAL("system started", SyslogMsg(lastMessage()).c_str());
}

TEST(SolidSyslog, MessageBodyIsPrecededByUtf8Bom)
{
    message.msg = "system started";
    Log();
    CHECK(SyslogMsgHasBom(lastMessage()));
}

TEST(SolidSyslog, CallerSuppliedBomIsStrippedSoOutputHasOnlyOne)
{
    message.msg = "\xEF\xBB\xBFsystem started";
    Log();
    STRCMP_EQUAL("system started", SyslogMsg(lastMessage()).c_str());
}

TEST(SolidSyslog, EmptyMessageOmitsMsgField)
{
    message.msg = "";
    Log();
    CHECK_FALSE(SyslogMsgHasBom(lastMessage()));
    STRCMP_EQUAL("", SyslogMsg(lastMessage()).c_str());
}

TEST(SolidSyslog, MessageBodyIsNotHardCoded)
{
    message.msg = TEST_MSG;
    Log();
    STRCMP_EQUAL(TEST_MSG, SyslogMsg(lastMessage()).c_str());
}

TEST(SolidSyslog, MessageWithSpacesIsPreserved)
{
    message.msg = "hello world with spaces";
    Log();
    STRCMP_EQUAL("hello world with spaces", SyslogMsg(lastMessage()).c_str());
}

TEST(SolidSyslog, MessageFillsRemainingBuffer)
{
    std::string header("<134>1 - - - - - - " + std::string(UTF8_BOM));
    size_t      maxMsg = SOLIDSYSLOG_MAX_MESSAGE_SIZE - header.size() - 1;
    std::string longMsg(maxMsg, 'X');
    message.msg = longMsg.c_str();
    Log();
    STRCMP_EQUAL(longMsg.c_str(), SyslogMsg(lastMessage()).c_str());
}

TEST(SolidSyslog, MessageTruncatedWhenExceedingBuffer)
{
    std::string header("<134>1 - - - - - - " + std::string(UTF8_BOM));
    size_t      maxMsg = SOLIDSYSLOG_MAX_MESSAGE_SIZE - header.size() - 1;
    std::string longMsg(maxMsg + 100, 'X');
    message.msg = longMsg.c_str();
    Log();
    std::string expected(maxMsg, 'X');
    STRCMP_EQUAL(expected.c_str(), SyslogMsg(lastMessage()).c_str());
}

TEST(SolidSyslog, BomIsPreservedWhenMessageBodyTruncates)
{
    /* When the body overflows the wire-frame budget, BoundedString clips
     * the body but the BOM — written before the body — must remain
     * present. Pins the FormatMsg ordering: BOM first, body second. */
    std::string longMsg(SOLIDSYSLOG_MAX_MESSAGE_SIZE, 'X');
    message.msg = longMsg.c_str();
    Log();
    CHECK(SyslogMsgHasBom(lastMessage()));
}

TEST(SolidSyslog, HugeMessageDoesNotCorruptMemory)
{
    std::string hugeMsg(10000, 'Z');
    message.msg = hugeMsg.c_str();
    Log();
    std::string result = SyslogMsg(lastMessage());
    CHECK(result.size() <= SOLIDSYSLOG_MAX_MESSAGE_SIZE);
}

// clang-format off
static const uint16_t TEST_YEAR        = 2026;
static const uint8_t  TEST_MONTH       = 4;
static const uint8_t  TEST_DAY         = 2;
static const uint8_t  TEST_HOUR        = 14;
static const uint8_t  TEST_MINUTE      = 30;
static const uint8_t  TEST_SECOND      = 7;
static const uint32_t TEST_MICROSECOND = 42;
static const int16_t  TEST_UTC_OFFSET  = 0;
// clang-format on

static struct SolidSyslogTimestamp stubTimestamp;

static void StubClock(struct SolidSyslogTimestamp* timestamp)
{
    *timestamp = stubTimestamp;
}

// clang-format off
TEST_GROUP_BASE(SolidSyslogTimestamp, TEST_GROUP_CppUTestGroupSolidSyslog)
{
    void setup() override
    {
        TEST_GROUP_CppUTestGroupSolidSyslog::setup();
        stubTimestamp = {TEST_YEAR, TEST_MONTH, TEST_DAY, TEST_HOUR, TEST_MINUTE, TEST_SECOND, TEST_MICROSECOND, TEST_UTC_OFFSET};
        config.clock = StubClock;
        SolidSyslog_Destroy();
        SolidSyslog_Create(&config);
    }
};

// clang-format on

TEST(SolidSyslogTimestamp, NullClockProducesNilvalue)
{
    config.clock = nullptr;
    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    Log();
    CHECK_TIMESTAMP_IS_NILVALUE();
}

TEST(SolidSyslogTimestamp, YearFormatsAsFourDigitZeroPadded)
{
    stubTimestamp.year = 2026;
    Log();
    CHECK_TIMESTAMP_YEAR("2026");
}

TEST(SolidSyslogTimestamp, MonthFormatsAsTwoDigitZeroPadded)
{
    stubTimestamp.month = 4;
    Log();
    CHECK_TIMESTAMP_MONTH("04");
}

TEST(SolidSyslogTimestamp, DayFormatsAsTwoDigitZeroPadded)
{
    stubTimestamp.day = 2;
    Log();
    CHECK_TIMESTAMP_DAY("02");
}

TEST(SolidSyslogTimestamp, HourFormatsAsTwoDigitZeroPadded)
{
    stubTimestamp.hour = 14;
    Log();
    CHECK_TIMESTAMP_HOUR("14");
}

TEST(SolidSyslogTimestamp, MinuteFormatsAsTwoDigitZeroPadded)
{
    stubTimestamp.minute = 30;
    Log();
    CHECK_TIMESTAMP_MINUTE("30");
}

TEST(SolidSyslogTimestamp, SecondFormatsAsTwoDigitZeroPadded)
{
    stubTimestamp.second = 7;
    Log();
    CHECK_TIMESTAMP_SECOND("07");
}

TEST(SolidSyslogTimestamp, MicrosecondFormatsAsSixDigitZeroPadded)
{
    stubTimestamp.microsecond = 42;
    Log();
    CHECK_TIMESTAMP_MICROSECOND(".000042");
}

TEST(SolidSyslogTimestamp, DateFieldsSeparatedByHyphen)
{
    Log();
    std::string timestamp = SyslogField(lastMessage(), SYSLOG_FIELD_TIMESTAMP);
    BYTES_EQUAL('-', timestamp.at(TIMESTAMP_DATE_SEPARATOR_1));
    BYTES_EQUAL('-', timestamp.at(TIMESTAMP_DATE_SEPARATOR_2));
}

TEST(SolidSyslogTimestamp, DateAndTimeSeparatedByT)
{
    Log();
    std::string timestamp = SyslogField(lastMessage(), SYSLOG_FIELD_TIMESTAMP);
    BYTES_EQUAL('T', timestamp.at(TIMESTAMP_DATE_TIME_SEPARATOR));
}

TEST(SolidSyslogTimestamp, TimeFieldsSeparatedByColon)
{
    Log();
    std::string timestamp = SyslogField(lastMessage(), SYSLOG_FIELD_TIMESTAMP);
    BYTES_EQUAL(':', timestamp.at(TIMESTAMP_TIME_SEPARATOR_1));
    BYTES_EQUAL(':', timestamp.at(TIMESTAMP_TIME_SEPARATOR_2));
}

TEST(SolidSyslogTimestamp, FractionalSecondsPrecededByDot)
{
    Log();
    std::string timestamp = SyslogField(lastMessage(), SYSLOG_FIELD_TIMESTAMP);
    BYTES_EQUAL('.', timestamp.at(TIMESTAMP_MICROSECOND_OFFSET));
}

TEST(SolidSyslogTimestamp, TimestampAppearsInCorrectMessageFieldPosition)
{
    Log();
    CHECK_TIMESTAMP("2026-04-02T14:30:07.000042Z");
}

TEST(SolidSyslogTimestamp, ZeroOffsetFormatsAsZ)
{
    stubTimestamp.utcOffsetMinutes = 0;
    Log();
    CHECK_TIMESTAMP_OFFSET("Z");
}

TEST(SolidSyslogTimestamp, PositiveOffsetFormatsAsPlusHHMM)
{
    stubTimestamp.utcOffsetMinutes = 330;
    Log();
    CHECK_TIMESTAMP_OFFSET("+05:30");
}

TEST(SolidSyslogTimestamp, NegativeOffsetFormatsAsMinusHHMM)
{
    stubTimestamp.utcOffsetMinutes = -300;
    Log();
    CHECK_TIMESTAMP_OFFSET("-05:00");
}

TEST(SolidSyslogTimestamp, YearZeroFormatsAs0000)
{
    stubTimestamp.year = 0;
    Log();
    CHECK_TIMESTAMP_YEAR("0000");
}

TEST(SolidSyslogTimestamp, Year9999FormatsAs9999)
{
    stubTimestamp.year = 9999;
    Log();
    CHECK_TIMESTAMP_YEAR("9999");
}

TEST(SolidSyslogTimestamp, Month1FormatsAs01)
{
    stubTimestamp.month = 1;
    Log();
    CHECK_TIMESTAMP_MONTH("01");
}

TEST(SolidSyslogTimestamp, Month12FormatsAs12)
{
    stubTimestamp.month = 12;
    Log();
    CHECK_TIMESTAMP_MONTH("12");
}

TEST(SolidSyslogTimestamp, Day1FormatsAs01)
{
    stubTimestamp.day = 1;
    Log();
    CHECK_TIMESTAMP_DAY("01");
}

TEST(SolidSyslogTimestamp, Day31FormatsAs31)
{
    stubTimestamp.day = 31;
    Log();
    CHECK_TIMESTAMP_DAY("31");
}

TEST(SolidSyslogTimestamp, Hour0FormatsAs00)
{
    stubTimestamp.hour = 0;
    Log();
    CHECK_TIMESTAMP_HOUR("00");
}

TEST(SolidSyslogTimestamp, Hour23FormatsAs23)
{
    stubTimestamp.hour = 23;
    Log();
    CHECK_TIMESTAMP_HOUR("23");
}

TEST(SolidSyslogTimestamp, Minute0FormatsAs00)
{
    stubTimestamp.minute = 0;
    Log();
    CHECK_TIMESTAMP_MINUTE("00");
}

TEST(SolidSyslogTimestamp, Minute59FormatsAs59)
{
    stubTimestamp.minute = 59;
    Log();
    CHECK_TIMESTAMP_MINUTE("59");
}

TEST(SolidSyslogTimestamp, Second0FormatsAs00)
{
    stubTimestamp.second = 0;
    Log();
    CHECK_TIMESTAMP_SECOND("00");
}

TEST(SolidSyslogTimestamp, Second59FormatsAs59)
{
    stubTimestamp.second = 59;
    Log();
    CHECK_TIMESTAMP_SECOND("59");
}

TEST(SolidSyslogTimestamp, Microsecond0FormatsAs000000)
{
    stubTimestamp.microsecond = 0;
    Log();
    CHECK_TIMESTAMP_MICROSECOND(".000000");
}

TEST(SolidSyslogTimestamp, Microsecond999999FormatsAs999999)
{
    stubTimestamp.microsecond = 999999;
    Log();
    CHECK_TIMESTAMP_MICROSECOND(".999999");
}

TEST(SolidSyslogTimestamp, UtcOffsetPlus840FormatsAsPlus1400)
{
    stubTimestamp.utcOffsetMinutes = 840;
    Log();
    CHECK_TIMESTAMP_OFFSET("+14:00");
}

TEST(SolidSyslogTimestamp, UtcOffsetMinus720FormatsAsMinus1200)
{
    stubTimestamp.utcOffsetMinutes = -720;
    Log();
    CHECK_TIMESTAMP_OFFSET("-12:00");
}

TEST(SolidSyslogTimestamp, Month0ProducesNilvalue)
{
    stubTimestamp.month = 0;
    Log();
    CHECK_TIMESTAMP_IS_NILVALUE();
}

TEST(SolidSyslogTimestamp, Month13ProducesNilvalue)
{
    stubTimestamp.month = 13;
    Log();
    CHECK_TIMESTAMP_IS_NILVALUE();
}

TEST(SolidSyslogTimestamp, Day0ProducesNilvalue)
{
    stubTimestamp.day = 0;
    Log();
    CHECK_TIMESTAMP_IS_NILVALUE();
}

TEST(SolidSyslogTimestamp, Day32ProducesNilvalue)
{
    stubTimestamp.day = 32;
    Log();
    CHECK_TIMESTAMP_IS_NILVALUE();
}

TEST(SolidSyslogTimestamp, Hour24ProducesNilvalue)
{
    stubTimestamp.hour = 24;
    Log();
    CHECK_TIMESTAMP_IS_NILVALUE();
}

TEST(SolidSyslogTimestamp, Minute60ProducesNilvalue)
{
    stubTimestamp.minute = 60;
    Log();
    CHECK_TIMESTAMP_IS_NILVALUE();
}

TEST(SolidSyslogTimestamp, Second60ProducesNilvalue)
{
    stubTimestamp.second = 60;
    Log();
    CHECK_TIMESTAMP_IS_NILVALUE();
}

TEST(SolidSyslogTimestamp, Microsecond1000000ProducesNilvalue)
{
    stubTimestamp.microsecond = 1000000;
    Log();
    CHECK_TIMESTAMP_IS_NILVALUE();
}

TEST(SolidSyslogTimestamp, UtcOffsetPlus841ProducesNilvalue)
{
    stubTimestamp.utcOffsetMinutes = 841;
    Log();
    CHECK_TIMESTAMP_IS_NILVALUE();
}

TEST(SolidSyslogTimestamp, UtcOffsetMinus721ProducesNilvalue)
{
    stubTimestamp.utcOffsetMinutes = -721;
    Log();
    CHECK_TIMESTAMP_IS_NILVALUE();
}

TEST(SolidSyslog, HostnameAt255CharsIsAccepted)
{
    std::string longHostname(255, 'H');
    StringFake_SetHostname(longHostname.c_str());
    Log();
    CHECK_HOSTNAME(longHostname.c_str());
}

TEST(SolidSyslog, HostnameAt256CharsIsTruncatedTo255)
{
    std::string longHostname(256, 'H');
    StringFake_SetHostname(longHostname.c_str());
    Log();
    std::string expected(255, 'H');
    CHECK_HOSTNAME(expected.c_str());
}

TEST(SolidSyslog, HostnameNonPrintableByteIsSubstitutedWithQuestionMark)
{
    StringFake_SetHostname("a\x01"
                           "b");
    Log();
    CHECK_HOSTNAME("a?b");
}

TEST(SolidSyslog, AppNameAt48CharsIsAccepted)
{
    std::string longAppName(48, 'A');
    StringFake_SetAppName(longAppName.c_str());
    Log();
    CHECK_APP_NAME(longAppName.c_str());
}

TEST(SolidSyslog, AppNameAt49CharsIsTruncatedTo48)
{
    std::string longAppName(49, 'A');
    StringFake_SetAppName(longAppName.c_str());
    Log();
    std::string expected(48, 'A');
    CHECK_APP_NAME(expected.c_str());
}

TEST(SolidSyslog, AppNameNonPrintableByteIsSubstitutedWithQuestionMark)
{
    StringFake_SetAppName("a\x7F"
                          "b");
    Log();
    CHECK_APP_NAME("a?b");
}

TEST(SolidSyslog, ProcessIdAt128CharsIsAccepted)
{
    std::string longProcessId(128, 'P');
    StringFake_SetProcessId(longProcessId.c_str());
    Log();
    CHECK_PROCID(longProcessId.c_str());
}

TEST(SolidSyslog, ProcessIdAt129CharsIsTruncatedTo128)
{
    std::string longProcessId(129, 'P');
    StringFake_SetProcessId(longProcessId.c_str());
    Log();
    std::string expected(128, 'P');
    CHECK_PROCID(expected.c_str());
}

TEST(SolidSyslog, ProcessIdNonPrintableByteIsSubstitutedWithQuestionMark)
{
    StringFake_SetProcessId("a\xC3"
                            "b");
    Log();
    CHECK_PROCID("a?b");
}

TEST(SolidSyslog, AllFieldsAtMaxLengthProducesValidMessage)
{
    std::string maxHostname(255, 'H');
    std::string maxAppName(48, 'A');
    std::string maxProcessId(128, 'P');
    StringFake_SetHostname(maxHostname.c_str());
    StringFake_SetAppName(maxAppName.c_str());
    StringFake_SetProcessId(maxProcessId.c_str());
    stubTimestamp = {9999, 12, 31, 23, 59, 59, 999999, 840};
    config.clock  = StubClock;
    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    message.facility = SOLIDSYSLOG_FACILITY_LOCAL7;
    message.severity = SOLIDSYSLOG_SEVERITY_DEBUG;
    Log();
    CHECK_PRIVAL("<191>");
    CHECK_TIMESTAMP("9999-12-31T23:59:59.999999+14:00");
    CHECK_HOSTNAME(maxHostname.c_str());
    CHECK_APP_NAME(maxAppName.c_str());
    CHECK_PROCID(maxProcessId.c_str());
}

TEST(SolidSyslog, EmptyHostnameProducesNilvalue)
{
    StringFake_SetHostname("");
    Log();
    CHECK_HOSTNAME("-");
}

TEST(SolidSyslog, EmptyAppNameProducesNilvalue)
{
    StringFake_SetAppName("");
    Log();
    CHECK_APP_NAME("-");
}

TEST(SolidSyslog, EmptyProcessIdProducesNilvalue)
{
    StringFake_SetProcessId("");
    Log();
    CHECK_PROCID("-");
}

TEST(SolidSyslog, ServiceSendsMessageReadFromBuffer)
{
    SolidSyslogBuffer* fakeBuffer    = BufferFake_Create();
    SolidSyslogConfig  serviceConfig = {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, store, nullptr, 0};

    SolidSyslog_Destroy();
    SolidSyslog_Create(&serviceConfig);

    SolidSyslogBuffer_Write(fakeBuffer, "test", 4);
    SenderFake_Reset(fakeSender);
    SolidSyslog_Service();

    LONGS_EQUAL(1, SenderFake_SendCount(fakeSender));
    STRCMP_EQUAL("test", SenderFake_LastBufferAsString(fakeSender));

    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    BufferFake_Destroy();
}

TEST(SolidSyslog, ServiceSendsBufferedMessageWithNullStore)
{
    SolidSyslogBuffer* fakeBuffer    = BufferFake_Create();
    SolidSyslogStore*  nullStore     = SolidSyslogNullStore_Create();
    SolidSyslogConfig  serviceConfig = {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, nullStore, nullptr, 0};

    SolidSyslog_Destroy();
    SolidSyslog_Create(&serviceConfig);

    SolidSyslogBuffer_Write(fakeBuffer, "test", 4);
    SenderFake_Reset(fakeSender);
    SolidSyslog_Service();

    LONGS_EQUAL(1, SenderFake_SendCount(fakeSender));
    STRCMP_EQUAL("test", SenderFake_LastBufferAsString(fakeSender));

    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    SolidSyslogNullStore_Destroy();
    BufferFake_Destroy();
}

TEST(SolidSyslog, ServiceSendsFromStoreWhenHasUnsent)
{
    SolidSyslogBuffer* fakeBuffer    = BufferFake_Create();
    SolidSyslogStore*  fakeStore     = StoreFake_Create();
    SolidSyslogConfig  serviceConfig = {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy();
    SolidSyslog_Create(&serviceConfig);

    SolidSyslogStore_Write(fakeStore, "stored", 6);
    SenderFake_Reset(fakeSender);
    SolidSyslog_Service();

    LONGS_EQUAL(1, SenderFake_SendCount(fakeSender));
    STRCMP_EQUAL("stored", SenderFake_LastBufferAsString(fakeSender));

    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

TEST(SolidSyslog, ServiceMarksSentAfterSuccessfulSend)
{
    SolidSyslogBuffer* fakeBuffer    = BufferFake_Create();
    SolidSyslogStore*  fakeStore     = StoreFake_Create();
    SolidSyslogConfig  serviceConfig = {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy();
    SolidSyslog_Create(&serviceConfig);

    SolidSyslogStore_Write(fakeStore, "stored", 6);
    SolidSyslog_Service();

    CHECK_FALSE(SolidSyslogStore_HasUnsent(fakeStore));

    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

TEST(SolidSyslog, ServiceDoesNotMarkSentOnSendFailure)
{
    SolidSyslogBuffer* fakeBuffer    = BufferFake_Create();
    SolidSyslogStore*  fakeStore     = StoreFake_Create();
    SolidSyslogConfig  serviceConfig = {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy();
    SolidSyslog_Create(&serviceConfig);

    SolidSyslogStore_Write(fakeStore, "stored", 6);
    SenderFake_FailNextSend(fakeSender);
    SolidSyslog_Service();

    CHECK_TRUE(SolidSyslogStore_HasUnsent(fakeStore));

    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

TEST(SolidSyslog, ServiceWritesBufferMessageToStore)
{
    SolidSyslogBuffer* fakeBuffer    = BufferFake_Create();
    SolidSyslogStore*  fakeStore     = StoreFake_Create();
    SolidSyslogConfig  serviceConfig = {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy();
    SolidSyslog_Create(&serviceConfig);

    SolidSyslogBuffer_Write(fakeBuffer, "buffered", 8);
    SenderFake_FailNextSend(fakeSender);
    SolidSyslog_Service();

    char   readData[512];
    size_t readSize = 0;
    SolidSyslogStore_ReadNextUnsent(fakeStore, readData, sizeof(readData), &readSize);
    LONGS_EQUAL(8, readSize);
    MEMCMP_EQUAL("buffered", readData, 8);

    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

TEST(SolidSyslog, ServiceSendsStoreMessageNotBufferMessage)
{
    SolidSyslogBuffer* fakeBuffer    = BufferFake_Create();
    SolidSyslogStore*  fakeStore     = StoreFake_Create();
    SolidSyslogConfig  serviceConfig = {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy();
    SolidSyslog_Create(&serviceConfig);

    SolidSyslogStore_Write(fakeStore, "old", 3);
    SolidSyslogBuffer_Write(fakeBuffer, "new", 3);
    SenderFake_Reset(fakeSender);
    SolidSyslog_Service();

    STRCMP_EQUAL("old", SenderFake_LastBufferAsString(fakeSender));

    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

TEST(SolidSyslog, ServiceSendsDirectlyWhenStoreWriteFails)
{
    SolidSyslogBuffer* fakeBuffer    = BufferFake_Create();
    SolidSyslogStore*  fakeStore     = StoreFake_Create();
    SolidSyslogConfig  serviceConfig = {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy();
    SolidSyslog_Create(&serviceConfig);

    SolidSyslogBuffer_Write(fakeBuffer, "direct", 6);
    StoreFake_FailNextWrite();
    SenderFake_Reset(fakeSender);
    SolidSyslog_Service();

    LONGS_EQUAL(1, SenderFake_SendCount(fakeSender));
    STRCMP_EQUAL("direct", SenderFake_LastBufferAsString(fakeSender));

    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

TEST(SolidSyslog, ServiceDoesNotSendWhenStoreReadFails)
{
    SolidSyslogBuffer* fakeBuffer    = BufferFake_Create();
    SolidSyslogStore*  fakeStore     = StoreFake_Create();
    SolidSyslogConfig  serviceConfig = {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy();
    SolidSyslog_Create(&serviceConfig);

    SolidSyslogStore_Write(fakeStore, "stored", 6);
    StoreFake_FailNextRead();
    SenderFake_Reset(fakeSender);
    SolidSyslog_Service();

    LONGS_EQUAL(0, SenderFake_SendCount(fakeSender));

    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

TEST(SolidSyslog, ServiceDoesNotMarkSentWhenSendingFromBuffer)
{
    SolidSyslogBuffer* fakeBuffer    = BufferFake_Create();
    SolidSyslogStore*  fakeStore     = StoreFake_Create();
    SolidSyslogConfig  serviceConfig = {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy();
    SolidSyslog_Create(&serviceConfig);

    SolidSyslogStore_Write(fakeStore, "in-store", 8);
    SolidSyslogStore_MarkSent(fakeStore);
    SolidSyslogBuffer_Write(fakeBuffer, "from-buffer", 11);
    SenderFake_Reset(fakeSender);
    SolidSyslog_Service();

    LONGS_EQUAL(1, SenderFake_SendCount(fakeSender));
    STRCMP_EQUAL("from-buffer", SenderFake_LastBufferAsString(fakeSender));

    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

/* Shared fixture for the eager-drain Service tests — both wire a real
 * CircularBuffer (drives the multi-message-per-tick path) and a FIFO
 * StoreFake. Storage is static so a CHECK failure that skips the test
 * body's cleanup cannot leave a dangling stack reference behind for
 * SolidSyslog_Destroy in teardown. */
// clang-format off
TEST_GROUP(SolidSyslogServiceEagerDrain)
{
    static constexpr size_t BUFFER_BYTES = 256;

    struct SolidSyslogSender* fakeSender     = nullptr;
    struct SolidSyslogBuffer* circularBuffer = nullptr;
    struct SolidSyslogStore*  fakeStore      = nullptr;

    void setup() override
    {
        static SolidSyslogCircularBufferStorage bufferStorage[
            SOLIDSYSLOG_CIRCULARBUFFER_STORAGE_SIZE_BYTES(BUFFER_BYTES)];

        fakeSender     = SenderFake_Create();
        circularBuffer = SolidSyslogCircularBuffer_Create(
            bufferStorage, sizeof(bufferStorage), SolidSyslogNullMutex_Create());
        fakeStore      = StoreFake_Create();

        SolidSyslogConfig serviceConfig = {};
        serviceConfig.buffer            = circularBuffer;
        serviceConfig.sender            = fakeSender;
        serviceConfig.store             = fakeStore;
        SolidSyslog_Create(&serviceConfig);
    }

    void teardown() override
    {
        SolidSyslog_Destroy();
        StoreFake_Destroy();
        SolidSyslogCircularBuffer_Destroy(circularBuffer);
        SolidSyslogNullMutex_Destroy();
        SenderFake_Destroy(fakeSender);
    }
};

// clang-format on

TEST(SolidSyslogServiceEagerDrain, AllBufferedMessagesReachStoreInOneTickWhenSenderFails)
{
    SolidSyslogBuffer_Write(circularBuffer, "msg1", 4);
    SolidSyslogBuffer_Write(circularBuffer, "msg2", 4);
    SolidSyslogBuffer_Write(circularBuffer, "msg3", 4);
    SenderFake_FailNextSend(fakeSender);
    SolidSyslog_Service();

    LONGS_EQUAL(3, StoreFake_WriteCount(fakeStore));
}

TEST(SolidSyslogServiceEagerDrain, StoredMessagesDrainInFifoOrderAcrossTicks)
{
    SolidSyslogBuffer_Write(circularBuffer, "m1", 2);
    SolidSyslogBuffer_Write(circularBuffer, "m2", 2);
    SolidSyslogBuffer_Write(circularBuffer, "m3", 2);

    SolidSyslog_Service();
    STRCMP_EQUAL("m1", SenderFake_LastBufferAsString(fakeSender));
    SolidSyslog_Service();
    STRCMP_EQUAL("m2", SenderFake_LastBufferAsString(fakeSender));
    SolidSyslog_Service();
    STRCMP_EQUAL("m3", SenderFake_LastBufferAsString(fakeSender));
    LONGS_EQUAL(3, SenderFake_SendCount(fakeSender));
}

TEST(SolidSyslog, ServiceDoesNothingWhenStoreIsHalted)
{
    SolidSyslogBuffer* fakeBuffer    = BufferFake_Create();
    SolidSyslogStore*  fakeStore     = StoreFake_Create();
    SolidSyslogConfig  serviceConfig = {fakeBuffer, fakeSender, nullptr, nullptr, nullptr, nullptr, fakeStore, nullptr, 0};

    SolidSyslog_Destroy();
    SolidSyslog_Create(&serviceConfig);

    SolidSyslogBuffer_Write(fakeBuffer, "test", 4);
    StoreFake_SetHalted();
    SenderFake_Reset(fakeSender);
    SolidSyslog_Service();

    LONGS_EQUAL(0, SenderFake_SendCount(fakeSender));

    SolidSyslog_Destroy();
    SolidSyslog_Create(&config);
    StoreFake_Destroy();
    BufferFake_Destroy();
}

TEST(SolidSyslog, LogAfterDestroyAndRecreateWithNullFunctionsProducesNilvalues)
{
    SolidSyslog_Destroy();
    SolidSyslogConfig nilConfig = {buffer, nullptr, nullptr, nullptr, nullptr, nullptr, store, nullptr, 0};
    SolidSyslog_Create(&nilConfig);
    Log();
    CHECK_TIMESTAMP_IS_NILVALUE();
    CHECK_HOSTNAME("-");
    CHECK_APP_NAME("-");
    CHECK_PROCID("-");
}

IGNORE_TEST(SolidSyslog, HappyPathOnly)

{
    // Error handling not yet implemented — see Epic #31
    //   SolidSyslog_Create with a NULL config does not crash
    //
    // Optional header fields not yet driven in — see Epic #8
    //   MSG is preceded by UTF-8 BOM
}
