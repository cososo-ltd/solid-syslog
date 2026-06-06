#include "BddTargetAppName.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogHeaderFieldPrivate.h"
#include "CppUTest/TestHarness.h"

enum
{
    FORMATTER_BUFFER_SIZE = 64
};

// clang-format off
TEST_GROUP(BddTargetAppName)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(FORMATTER_BUFFER_SIZE)];
    struct SolidSyslogFormatter* formatter = nullptr;
    struct SolidSyslogHeaderField field{};

    void setup() override
    {
        formatter = SolidSyslogFormatter_Create(storage, FORMATTER_BUFFER_SIZE);
        SolidSyslogHeaderField_FromFormatter(&field, formatter, FORMATTER_BUFFER_SIZE);
    }

    [[nodiscard]] const char* formatted() const
    {
        return SolidSyslogFormatter_AsFormattedBuffer(formatter);
    }
};

// clang-format on

TEST(BddTargetAppName, BackslashSeparatorExtractsBaseName)
{
    BddTargetAppName_Set("dir\\binary");
    BddTargetAppName_Get(&field, nullptr);
    STRCMP_EQUAL("binary", formatted());
}

TEST(BddTargetAppName, ForwardSlashSeparatorExtractsBaseName)
{
    BddTargetAppName_Set("/usr/local/bin/example");
    BddTargetAppName_Get(&field, nullptr);
    STRCMP_EQUAL("example", formatted());
}

TEST(BddTargetAppName, NoSeparatorReturnsWholeArgument)
{
    BddTargetAppName_Set("example");
    BddTargetAppName_Get(&field, nullptr);
    STRCMP_EQUAL("example", formatted());
}

TEST(BddTargetAppName, MixedSeparatorsUseRightmost)
{
    BddTargetAppName_Set("C:\\msys64\\home/user/example");
    BddTargetAppName_Get(&field, nullptr);
    STRCMP_EQUAL("example", formatted());
}

TEST(BddTargetAppName, ExeExtensionIsStripped)
{
    BddTargetAppName_Set("app.exe");
    BddTargetAppName_Get(&field, nullptr);
    STRCMP_EQUAL("app", formatted());
}

TEST(BddTargetAppName, UpperCaseExeExtensionIsStripped)
{
    BddTargetAppName_Set("APP.EXE");
    BddTargetAppName_Get(&field, nullptr);
    STRCMP_EQUAL("APP", formatted());
}

TEST(BddTargetAppName, ExeWithPathSeparatorIsStripped)
{
    BddTargetAppName_Set("C:\\bin\\SolidSyslogBddTarget.exe");
    BddTargetAppName_Get(&field, nullptr);
    STRCMP_EQUAL("SolidSyslogBddTarget", formatted());
}

TEST(BddTargetAppName, NonExeExtensionIsKept)
{
    BddTargetAppName_Set("data.txt");
    BddTargetAppName_Get(&field, nullptr);
    STRCMP_EQUAL("data.txt", formatted());
}
