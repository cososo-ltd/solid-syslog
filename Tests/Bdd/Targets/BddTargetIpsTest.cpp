#include "BddTargetIps.h"
#include "SolidSyslogFormatter.h"
#include "CppUTest/TestHarness.h"

enum
{
    FORMATTER_BUFFER_SIZE = 64
};

// clang-format off
TEST_GROUP(BddTargetIps)
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

TEST(BddTargetIps, CountIsOne)
{
    LONGS_EQUAL(1, BddTargetIps_Count());
}

TEST(BddTargetIps, AtZeroEmitsTheConfiguredIp)
{
    BddTargetIps_At(formatter, 0);
    STRCMP_EQUAL("192.0.2.1", formatted());
}
