#include "CppUTest/TestHarness.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogNullSd.h"
#include "SolidSyslogStructuredData.h"

// clang-format off
TEST_GROUP(SolidSyslogNullSd)
{
    struct SolidSyslogStructuredData* sd = nullptr;
    SolidSyslogFormatterStorage formatterStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(32)];
    struct SolidSyslogFormatter* formatter = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        sd = SolidSyslogNullSd_Get();
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        formatter = SolidSyslogFormatter_Create(formatterStorage, 32);
    }
};

// clang-format on

TEST(SolidSyslogNullSd, FormatWritesNothing)
{
    SolidSyslogStructuredData_Format(sd, formatter);
    LONGS_EQUAL(0, SolidSyslogFormatter_Length(formatter));
}
