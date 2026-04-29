#include "CppUTest/TestHarness.h"
#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogStructuredData.h"
#include "TestAtomicOps.h"

#include <cstdint>
#include <cstring>

enum
{
    TEST_BUFFER_SIZE = 256
};

static uint32_t fakeSysUpTimeValue;

static uint32_t FakeSysUpTime_Get()
{
    return fakeSysUpTimeValue;
}

// clang-format off
TEST_GROUP(SolidSyslogMetaSd)
{
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    SolidSyslogAtomicCounter* counter;
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    SolidSyslogStructuredData* sd;
    SolidSyslogMetaSdConfig config;
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(TEST_BUFFER_SIZE)];
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    SolidSyslogFormatter* formatter;

    void setup() override
    {
        formatter = SolidSyslogFormatter_Create(storage, TEST_BUFFER_SIZE);
        counter = SolidSyslogAtomicCounter_Create(TestAtomicOps_Create());
        fakeSysUpTimeValue = 0;
        config = {};
        config.counter = counter;
        sd = SolidSyslogMetaSd_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogMetaSd_Destroy();
        SolidSyslogAtomicCounter_Destroy();
        TestAtomicOps_Destroy();
    }

    void recreate()
    {
        SolidSyslogMetaSd_Destroy();
        sd = SolidSyslogMetaSd_Create(&config);
    }

    void format() const
    {
        SolidSyslogStructuredData_Format(sd, formatter);
    }

    void resetFormatter()
    {
        formatter = SolidSyslogFormatter_Create(storage, TEST_BUFFER_SIZE);
    }
};

// clang-format on

TEST(SolidSyslogMetaSd, CreateReturnsNonNull)
{
    CHECK(sd != nullptr);
}

TEST(SolidSyslogMetaSd, FirstFormatProducesSequenceId1)
{
    format();
    STRCMP_EQUAL("[meta sequenceId=\"1\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogMetaSd, SecondFormatProducesSequenceId2)
{
    format();
    resetFormatter();
    format();
    STRCMP_EQUAL("[meta sequenceId=\"2\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogMetaSd, ThirdFormatProducesSequenceId3)
{
    format();
    format();
    resetFormatter();
    format();
    STRCMP_EQUAL("[meta sequenceId=\"3\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogMetaSd, FormatAdvancesFormatterLength)
{
    LONGS_EQUAL(0, SolidSyslogFormatter_Length(formatter));
    format();
    CHECK(SolidSyslogFormatter_Length(formatter) > 0);
    LONGS_EQUAL(strlen(SolidSyslogFormatter_AsFormattedBuffer(formatter)), SolidSyslogFormatter_Length(formatter));
}

TEST(SolidSyslogMetaSd, DestroyDoesNotCrash)
{
    // Covered by teardown — this test documents the intent
}

TEST(SolidSyslogMetaSd, FormatIncludesSysUpTimeFromCallback)
{
    fakeSysUpTimeValue  = 12345;
    config.getSysUpTime = FakeSysUpTime_Get;
    recreate();
    format();
    STRCMP_EQUAL("[meta sequenceId=\"1\" sysUpTime=\"12345\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogMetaSd, FormatIncludesDifferentSysUpTimeFromCallback)
{
    fakeSysUpTimeValue  = 99999;
    config.getSysUpTime = FakeSysUpTime_Get;
    recreate();
    format();
    STRCMP_EQUAL("[meta sequenceId=\"1\" sysUpTime=\"99999\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogMetaSd, FormatIncludesSysUpTimeAtZero)
{
    fakeSysUpTimeValue  = 0;
    config.getSysUpTime = FakeSysUpTime_Get;
    recreate();
    format();
    STRCMP_EQUAL("[meta sequenceId=\"1\" sysUpTime=\"0\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogMetaSd, FormatIncludesSysUpTimeAtMaxUint32)
{
    fakeSysUpTimeValue  = UINT32_MAX;
    config.getSysUpTime = FakeSysUpTime_Get;
    recreate();
    format();
    STRCMP_EQUAL("[meta sequenceId=\"1\" sysUpTime=\"4294967295\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}
