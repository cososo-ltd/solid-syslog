#include "SolidSyslogFormatter.h"
#include "StringFake.h"
#include "CppUTest/TestHarness.h"

struct SolidSyslogFormatter;

enum
{
    TEST_BUFFER_SIZE = 32
};

// clang-format off
TEST_GROUP(StringFake)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(TEST_BUFFER_SIZE)];
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    SolidSyslogFormatter* formatter;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- formatter is used across TEST() bodies via CppUTest macro
        formatter = SolidSyslogFormatter_Create(storage, TEST_BUFFER_SIZE);
        StringFake_Reset();
    }
};

// clang-format on

TEST(StringFake, ReturnsEmptyStringAfterReset)
{
    StringFake_GetHostname(formatter);
    STRCMP_EQUAL("", SolidSyslogFormatter_AsFormattedBuffer(formatter));
    LONGS_EQUAL(0, SolidSyslogFormatter_Length(formatter));
}

TEST(StringFake, ReturnsConfiguredHostname)
{
    StringFake_SetHostname("MyHost");
    StringFake_GetHostname(formatter);
    STRCMP_EQUAL("MyHost", SolidSyslogFormatter_AsFormattedBuffer(formatter));
    LONGS_EQUAL(6, SolidSyslogFormatter_Length(formatter));
}
