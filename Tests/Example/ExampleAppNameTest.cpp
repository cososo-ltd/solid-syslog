#include "ExampleAppName.h"
#include "SolidSyslogFormatter.h"
#include "CppUTest/TestHarness.h"

enum
{
    FORMATTER_BUFFER_SIZE = 64
};

// clang-format off
TEST_GROUP(ExampleAppName)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(FORMATTER_BUFFER_SIZE)];
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogFormatter* formatter = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        formatter = SolidSyslogFormatter_Create(storage, FORMATTER_BUFFER_SIZE);
    }

    [[nodiscard]] const char* formatted() const
    {
        return SolidSyslogFormatter_AsFormattedBuffer(formatter);
    }
};

// clang-format on

TEST(ExampleAppName, BackslashSeparatorExtractsBaseName)
{
    ExampleAppName_Set("dir\\binary");
    ExampleAppName_Get(formatter);
    STRCMP_EQUAL("binary", formatted());
}

TEST(ExampleAppName, ForwardSlashSeparatorExtractsBaseName)
{
    ExampleAppName_Set("/usr/local/bin/example");
    ExampleAppName_Get(formatter);
    STRCMP_EQUAL("example", formatted());
}

TEST(ExampleAppName, NoSeparatorReturnsWholeArgument)
{
    ExampleAppName_Set("example");
    ExampleAppName_Get(formatter);
    STRCMP_EQUAL("example", formatted());
}

TEST(ExampleAppName, MixedSeparatorsUseRightmost)
{
    ExampleAppName_Set("C:\\msys64\\home/user/example");
    ExampleAppName_Get(formatter);
    STRCMP_EQUAL("example", formatted());
}

TEST(ExampleAppName, ExeExtensionIsStripped)
{
    ExampleAppName_Set("app.exe");
    ExampleAppName_Get(formatter);
    STRCMP_EQUAL("app", formatted());
}

TEST(ExampleAppName, UpperCaseExeExtensionIsStripped)
{
    ExampleAppName_Set("APP.EXE");
    ExampleAppName_Get(formatter);
    STRCMP_EQUAL("APP", formatted());
}

TEST(ExampleAppName, ExeWithPathSeparatorIsStripped)
{
    ExampleAppName_Set("C:\\bin\\SolidSyslogExample.exe");
    ExampleAppName_Get(formatter);
    STRCMP_EQUAL("SolidSyslogExample", formatted());
}

TEST(ExampleAppName, NonExeExtensionIsKept)
{
    ExampleAppName_Set("data.txt");
    ExampleAppName_Get(formatter);
    STRCMP_EQUAL("data.txt", formatted());
}
