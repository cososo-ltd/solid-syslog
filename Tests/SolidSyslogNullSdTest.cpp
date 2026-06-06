#include "CppUTest/TestHarness.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogNullSd.h"
#include "SolidSyslogSdElementPrivate.h"
#include "SolidSyslogStructuredData.h"

// clang-format off
TEST_GROUP(SolidSyslogNullSd)
{
    struct SolidSyslogStructuredData* sd = nullptr;
    SolidSyslogFormatterStorage formatterStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(32)];
    struct SolidSyslogFormatter* formatter = nullptr;

    void setup() override
    {
        sd = SolidSyslogNullSd_Get();
        formatter = SolidSyslogFormatter_Create(formatterStorage, 32);
    }
};

// clang-format on

TEST(SolidSyslogNullSd, FormatWritesNothing)
{
    struct SolidSyslogSdElement element{};
    SolidSyslogSdElement_FromFormatter(&element, formatter);
    SolidSyslogStructuredData_Format(sd, &element);
    LONGS_EQUAL(0, SolidSyslogFormatter_Length(formatter));
}
