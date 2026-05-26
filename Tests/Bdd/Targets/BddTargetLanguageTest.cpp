#include "BddTargetLanguage.h"
#include "SolidSyslogFormatter.h"
#include "CppUTest/TestHarness.h"

enum
{
    FORMATTER_BUFFER_SIZE = 64
};

// clang-format off
TEST_GROUP(BddTargetLanguage)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(FORMATTER_BUFFER_SIZE)];
    struct SolidSyslogFormatter* formatter = nullptr;

    void setup() override
    {
        formatter = SolidSyslogFormatter_Create(storage, FORMATTER_BUFFER_SIZE);
    }

    [[nodiscard]] const char* formatted() const
    {
        return SolidSyslogFormatter_AsFormattedBuffer(formatter);
    }
};

// clang-format on

TEST(BddTargetLanguage, EmitsBritishEnglishTag)
{
    BddTargetLanguage_Get(formatter);
    STRCMP_EQUAL("en-GB", formatted());
}
