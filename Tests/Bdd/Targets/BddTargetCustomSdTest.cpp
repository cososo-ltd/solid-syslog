#include "BddTargetCustomSd.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogSdElement.h"
#include "SolidSyslogSdElementPrivate.h"
#include "SolidSyslogStructuredData.h"

#include "CppUTest/TestHarness.h"

enum
{
    FORMATTER_BUFFER_SIZE = 128
};

// clang-format off
TEST_GROUP(BddTargetCustomSd)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(FORMATTER_BUFFER_SIZE)];
    struct SolidSyslogFormatter* formatter = nullptr;
    struct SolidSyslogSdElement element{};

    void setup() override
    {
        formatter = SolidSyslogFormatter_Create(storage, FORMATTER_BUFFER_SIZE);
        SolidSyslogSdElement_FromFormatter(&element, formatter);
    }

    [[nodiscard]] const char* formatted() const
    {
        return SolidSyslogFormatter_AsFormattedBuffer(formatter);
    }
};

// clang-format on

TEST(BddTargetCustomSd, EmitsTheExampleElement)
{
    SolidSyslogStructuredData_Format(BddTargetCustomSd_Get(), &element);
    STRCMP_EQUAL("[example@32473 detail=\"Hello World\"]", formatted());
}
