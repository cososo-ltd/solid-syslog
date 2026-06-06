#include <cstring>
#include <stdint.h>

#include "CppUTest/TestHarness.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogSdElement.h"
#include "SolidSyslogSdElementPrivate.h"
#include "SolidSyslogSdValue.h"

enum
{
    TEST_BUFFER_SIZE = 128
};

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)
#define CHECK_FRAMED(expected)                                                     \
    do                                                                             \
    {                                                                              \
        STRCMP_EQUAL(expected, SolidSyslogFormatter_AsFormattedBuffer(formatter)); \
    } while (0)

// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

// clang-format off
TEST_GROUP(SolidSyslogSdElement)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(TEST_BUFFER_SIZE)];
    struct SolidSyslogFormatter* formatter = nullptr;
    struct SolidSyslogSdElement element{};

    void setup() override
    {
        formatter = SolidSyslogFormatter_Create(storage, TEST_BUFFER_SIZE);
        SolidSyslogSdElement_FromFormatter(&element, formatter);
    }
};

// clang-format on

TEST(SolidSyslogSdElement, BeginEmitsOpenBracketAndIanaName)
{
    SolidSyslogSdElement_Begin(&element, "meta", 0);

    CHECK_FRAMED("[meta");
}

TEST(SolidSyslogSdElement, BeginEmitsNameWithEnterpriseNumber)
{
    SolidSyslogSdElement_Begin(&element, "ex", 32473);

    CHECK_FRAMED("[ex@32473");
}

TEST(SolidSyslogSdElement, ParamOpensSpaceNameEqualsQuote)
{
    SolidSyslogSdElement_Begin(&element, "meta", 0);
    SolidSyslogSdElement_Param(&element, "tzKnown");

    CHECK_FRAMED("[meta tzKnown=\"");
}

TEST(SolidSyslogSdElement, ParamReturnsValueSinkStreamingIntoTheElement)
{
    SolidSyslogSdElement_Begin(&element, "meta", 0);
    struct SolidSyslogSdValue* value = SolidSyslogSdElement_Param(&element, "tzKnown");
    SolidSyslogSdValue_String(value, "1");

    CHECK_FRAMED("[meta tzKnown=\"1");
}

TEST(SolidSyslogSdElement, EndClosesValueQuoteAndElement)
{
    SolidSyslogSdElement_Begin(&element, "meta", 0);
    struct SolidSyslogSdValue* value = SolidSyslogSdElement_Param(&element, "tzKnown");
    SolidSyslogSdValue_String(value, "1");
    SolidSyslogSdElement_End(&element);

    CHECK_FRAMED("[meta tzKnown=\"1\"]");
}

TEST(SolidSyslogSdElement, EndWithNoParamClosesElementOnly)
{
    SolidSyslogSdElement_Begin(&element, "meta", 0);
    SolidSyslogSdElement_End(&element);

    CHECK_FRAMED("[meta]");
}

TEST(SolidSyslogSdElement, SecondParamClosesThePreviousValueQuote)
{
    SolidSyslogSdElement_Begin(&element, "timeQuality", 0);
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(&element, "tzKnown"), "1");
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(&element, "isSynced"), "0");
    SolidSyslogSdElement_End(&element);

    CHECK_FRAMED("[timeQuality tzKnown=\"1\" isSynced=\"0\"]");
}

TEST(SolidSyslogSdElement, BeginWithNullNameSuppressesTheWholeElement)
{
    /* A NULL SD-ID can never form a well-formed element, so the whole element
     * is skipped — params and close emit nothing, writes are absorbed safely. */
    SolidSyslogSdElement_Begin(&element, nullptr, 0);
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(&element, "p"), "v");
    SolidSyslogSdElement_End(&element);

    CHECK_FRAMED("");
}

TEST(SolidSyslogSdElement, ParamWithNullNameIsSkippedButElementStands)
{
    SolidSyslogSdElement_Begin(&element, "meta", 0);
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(&element, nullptr), "v");
    SolidSyslogSdElement_End(&element);

    CHECK_FRAMED("[meta]");
}

TEST(SolidSyslogSdElement, SkippedParamDoesNotDisturbSurroundingParams)
{
    SolidSyslogSdElement_Begin(&element, "meta", 0);
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(&element, "a"), "1");
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(&element, nullptr), "x");
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(&element, "b"), "2");
    SolidSyslogSdElement_End(&element);

    CHECK_FRAMED("[meta a=\"1\" b=\"2\"]");
}

TEST(SolidSyslogSdElement, ParamValueIsEscapedThroughTheValueSink)
{
    /* The element owns no escaping path of its own — the value's '"' is escaped
     * by the reused SolidSyslogSdValue, so framing stays intact. */
    SolidSyslogSdElement_Begin(&element, "meta", 0);
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(&element, "p"), "a\"b");
    SolidSyslogSdElement_End(&element);

    CHECK_FRAMED("[meta p=\"a\\\"b\"]");
}
