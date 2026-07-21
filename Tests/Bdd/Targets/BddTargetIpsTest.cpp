#include "BddTargetIps.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogSdValue.h"
#include "SolidSyslogSdValuePrivate.h"
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
    struct SolidSyslogSdValue value{};

    void setup() override
    {
        formatter = SolidSyslogFormatter_Create(storage, FORMATTER_BUFFER_SIZE);
        SolidSyslogSdValue_FromFormatter(&value, formatter);
    }

    [[nodiscard]] const char* formatted() const
    {
        return SolidSyslogFormatter_AsFormattedBuffer(formatter);
    }
};

// clang-format on

TEST(BddTargetIps, CountIsOne)
{
    LONGS_EQUAL(1, BddTargetIps_Count(nullptr));
}

TEST(BddTargetIps, AtZeroEmitsTheConfiguredIp)
{
    BddTargetIps_At(&value, nullptr, 0);
    SolidSyslogSdValue_Close(&value);
    STRCMP_EQUAL("192.0.2.1", formatted());
}
