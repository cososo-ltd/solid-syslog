#include <cstdint>
#include <cstring>

#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogStructuredData.h"
#include "CppUTest/TestHarness.h"

class TEST_SolidSyslogMetaSd_FirstFormatProducesSequenceId1_Test;
class TEST_SolidSyslogMetaSd_FormatEscapesBackslashInLanguage_Test;
class TEST_SolidSyslogMetaSd_FormatEscapesBracketInLanguage_Test;
class TEST_SolidSyslogMetaSd_FormatEscapesQuoteInLanguage_Test;
class TEST_SolidSyslogMetaSd_FormatIncludesDifferentLanguageFromCallback_Test;
class TEST_SolidSyslogMetaSd_FormatIncludesDifferentSysUpTimeFromCallback_Test;
class TEST_SolidSyslogMetaSd_FormatIncludesLanguageFromCallback_Test;
class TEST_SolidSyslogMetaSd_FormatIncludesSysUpTimeAtMaxUint32_Test;
class TEST_SolidSyslogMetaSd_FormatIncludesSysUpTimeAtZero_Test;
class TEST_SolidSyslogMetaSd_FormatIncludesSysUpTimeFromCallback_Test;
class TEST_SolidSyslogMetaSd_SecondFormatProducesSequenceId2_Test;
class TEST_SolidSyslogMetaSd_ThirdFormatProducesSequenceId3_Test;
struct SolidSyslogAtomicCounter;
struct SolidSyslogFormatter;
struct SolidSyslogStructuredData;

enum
{
    TEST_BUFFER_SIZE = 256
};

static uint32_t fakeSysUpTimeValue;

static uint32_t FakeSysUpTime_Get()
{
    return fakeSysUpTimeValue;
}

static const char* fakeLanguageContent;
static size_t      fakeLanguageMaxLength;

static void FakeLanguage_Get(struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_EscapedString(formatter, fakeLanguageContent, fakeLanguageMaxLength);
}

// NOLINTBEGIN(cppcoreguidelines-macro-usage) -- macros preserve __FILE__/__LINE__ in test failure output
#define CHECK_SEQUENCEID(expected) STRCMP_EQUAL("[meta sequenceId=\"" expected "\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter))
#define CHECK_SYSUPTIME(expected) STRCMP_EQUAL("[meta sequenceId=\"1\" sysUpTime=\"" expected "\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter))
#define CHECK_LANGUAGE(expected) STRCMP_EQUAL("[meta sequenceId=\"1\" language=\"" expected "\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter))

// NOLINTEND(cppcoreguidelines-macro-usage)

// clang-format off
TEST_GROUP(SolidSyslogMetaSd)
{
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    SolidSyslogAtomicCounter* counter;
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    SolidSyslogStructuredData* sd;
    SolidSyslogMetaSdConfig config;
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(TEST_BUFFER_SIZE)];
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    SolidSyslogFormatter* formatter;

    void setup() override
    {
        formatter = SolidSyslogFormatter_Create(storage, TEST_BUFFER_SIZE);
        counter = SolidSyslogAtomicCounter_Create();
        fakeSysUpTimeValue = 0;
        fakeLanguageContent = nullptr;
        fakeLanguageMaxLength = 0;
        config = {};
        config.counter = counter;
        sd = SolidSyslogMetaSd_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogMetaSd_Destroy();
        SolidSyslogAtomicCounter_Destroy();
    }

    void recreate()
    {
        SolidSyslogMetaSd_Destroy();
        sd = SolidSyslogMetaSd_Create(&config);
    }

    void useSysUpTime(uint32_t value)
    {
        fakeSysUpTimeValue = value;
        config.getSysUpTime = FakeSysUpTime_Get;
        recreate();
    }

    void useLanguage(const char* tag)
    {
        fakeLanguageContent = tag;
        fakeLanguageMaxLength = strlen(tag);
        config.getLanguage = FakeLanguage_Get;
        recreate();
    }

    void format() const
    {
        SolidSyslogStructuredData_Format(sd, formatter);
    }

    void resetFormatter()
    {
        formatter = SolidSyslogFormatter_Create(storage, TEST_BUFFER_SIZE);
    }
};

// clang-format on

TEST(SolidSyslogMetaSd, CreateReturnsNonNull)
{
    CHECK(sd != nullptr);
}

TEST(SolidSyslogMetaSd, FirstFormatProducesSequenceId1)
{
    format();
    CHECK_SEQUENCEID("1");
}

TEST(SolidSyslogMetaSd, SecondFormatProducesSequenceId2)
{
    format();
    resetFormatter();
    format();
    CHECK_SEQUENCEID("2");
}

TEST(SolidSyslogMetaSd, ThirdFormatProducesSequenceId3)
{
    format();
    format();
    resetFormatter();
    format();
    CHECK_SEQUENCEID("3");
}

TEST(SolidSyslogMetaSd, FormatAdvancesFormatterLength)
{
    LONGS_EQUAL(0, SolidSyslogFormatter_Length(formatter));
    format();
    CHECK(SolidSyslogFormatter_Length(formatter) > 0);
    LONGS_EQUAL(strlen(SolidSyslogFormatter_AsFormattedBuffer(formatter)), SolidSyslogFormatter_Length(formatter));
}

TEST(SolidSyslogMetaSd, DestroyDoesNotCrash)
{
    // Covered by teardown — this test documents the intent
}

TEST(SolidSyslogMetaSd, FormatIncludesSysUpTimeFromCallback)
{
    useSysUpTime(12345);
    format();
    CHECK_SYSUPTIME("12345");
}

TEST(SolidSyslogMetaSd, FormatIncludesDifferentSysUpTimeFromCallback)
{
    useSysUpTime(99999);
    format();
    CHECK_SYSUPTIME("99999");
}

TEST(SolidSyslogMetaSd, FormatIncludesSysUpTimeAtZero)
{
    useSysUpTime(0);
    format();
    CHECK_SYSUPTIME("0");
}

TEST(SolidSyslogMetaSd, FormatIncludesSysUpTimeAtMaxUint32)
{
    useSysUpTime(UINT32_MAX);
    format();
    CHECK_SYSUPTIME("4294967295");
}

TEST(SolidSyslogMetaSd, FormatIncludesLanguageFromCallback)
{
    useLanguage("en-GB");
    format();
    CHECK_LANGUAGE("en-GB");
}

TEST(SolidSyslogMetaSd, FormatIncludesDifferentLanguageFromCallback)
{
    useLanguage("fr");
    format();
    CHECK_LANGUAGE("fr");
}

TEST(SolidSyslogMetaSd, FormatEscapesQuoteInLanguage)
{
    useLanguage("a\"b");
    format();
    CHECK_LANGUAGE("a\\\"b");
}

TEST(SolidSyslogMetaSd, FormatEscapesBackslashInLanguage)
{
    useLanguage("a\\b");
    format();
    CHECK_LANGUAGE("a\\\\b");
}

TEST(SolidSyslogMetaSd, FormatEscapesBracketInLanguage)
{
    useLanguage("a]b");
    format();
    CHECK_LANGUAGE("a\\]b");
}

TEST(SolidSyslogMetaSd, FormatWithAllThreeParamsEmitsAllThree)
{
    useSysUpTime(12345);
    useLanguage("en-GB");
    format();
    STRCMP_EQUAL("[meta sequenceId=\"1\" sysUpTime=\"12345\" language=\"en-GB\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogMetaSd, FormatWithoutCounterOmitsSequenceId)
{
    config.counter = nullptr;
    useSysUpTime(12345);
    useLanguage("en-GB");
    format();
    STRCMP_EQUAL("[meta sysUpTime=\"12345\" language=\"en-GB\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogMetaSd, FormatWithNoConfiguredParamsEmitsBareMetaElement)
{
    config.counter = nullptr;
    recreate();
    format();
    STRCMP_EQUAL("[meta]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}
