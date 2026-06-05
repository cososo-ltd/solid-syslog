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

#define CHECK_FRAMED(expected) STRCMP_EQUAL(expected, SolidSyslogFormatter_AsFormattedBuffer(formatter))

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
