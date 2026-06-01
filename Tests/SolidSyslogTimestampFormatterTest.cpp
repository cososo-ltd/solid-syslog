#include <cstring>
#include <stdint.h>

#include "CppUTest/TestHarness.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogTimestamp.h"
#include "SolidSyslogTimestampFormatter.h"

enum
{
    TEST_BUFFER_SIZE = 64
};

static const uint16_t TEST_YEAR = 2026;
static const uint8_t TEST_MONTH = 4;
static const uint8_t TEST_DAY = 2;
static const uint8_t TEST_HOUR = 14;
static const uint8_t TEST_MINUTE = 30;
static const uint8_t TEST_SECOND = 7;
static const uint32_t TEST_MICROSECOND = 42;
static const int16_t TEST_UTC_OFFSET = 0;

#define CHECK_FORMATTED(expected)                                              \
    STRCMP_EQUAL(expected, SolidSyslogFormatter_AsFormattedBuffer(formatter)); \
    LONGS_EQUAL(strlen(expected), SolidSyslogFormatter_Length(formatter))

TEST_GROUP(SolidSyslogTimestampFormatter)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(TEST_BUFFER_SIZE)];
    struct SolidSyslogFormatter* formatter;
    struct SolidSyslogTimestamp ts;

    void setup() override
    {
        formatter = SolidSyslogFormatter_Create(storage, TEST_BUFFER_SIZE);
        ts = {TEST_YEAR, TEST_MONTH, TEST_DAY, TEST_HOUR, TEST_MINUTE, TEST_SECOND, TEST_MICROSECOND, TEST_UTC_OFFSET};
    }

    void format()
    {
        SolidSyslogTimestampFormatter_Format(formatter, &ts);
    }
};

// A full-string oracle on the canonical value proves every separator and
// every zero-padded field at once — strictly stronger than per-field checks.
TEST(SolidSyslogTimestampFormatter, FormatsValidTimestampAsRfc3339WithZuluOffset)
{
    format();

    CHECK_FORMATTED("2026-04-02T14:30:07.000042Z");
}

TEST(SolidSyslogTimestampFormatter, ZeroInitialisedTimestampProducesNilvalue)
{
    ts = {};

    format();

    CHECK_FORMATTED("-");
}

TEST(SolidSyslogTimestampFormatter, YearZeroFormatsAs0000)
{
    ts.Year = 0;

    format();

    CHECK_FORMATTED("0000-04-02T14:30:07.000042Z");
}

TEST(SolidSyslogTimestampFormatter, Year9999FormatsAs9999)
{
    ts.Year = 9999;

    format();

    CHECK_FORMATTED("9999-04-02T14:30:07.000042Z");
}

TEST(SolidSyslogTimestampFormatter, Month1FormatsAsZeroPadded01)
{
    ts.Month = 1;

    format();

    CHECK_FORMATTED("2026-01-02T14:30:07.000042Z");
}

TEST(SolidSyslogTimestampFormatter, Month12FormatsAs12)
{
    ts.Month = 12;

    format();

    CHECK_FORMATTED("2026-12-02T14:30:07.000042Z");
}

TEST(SolidSyslogTimestampFormatter, Month0ProducesNilvalue)
{
    ts.Month = 0;

    format();

    CHECK_FORMATTED("-");
}

TEST(SolidSyslogTimestampFormatter, Month13ProducesNilvalue)
{
    ts.Month = 13;

    format();

    CHECK_FORMATTED("-");
}

TEST(SolidSyslogTimestampFormatter, Day1FormatsAsZeroPadded01)
{
    ts.Day = 1;

    format();

    CHECK_FORMATTED("2026-04-01T14:30:07.000042Z");
}

TEST(SolidSyslogTimestampFormatter, Day31FormatsAs31)
{
    ts.Day = 31;

    format();

    CHECK_FORMATTED("2026-04-31T14:30:07.000042Z");
}

TEST(SolidSyslogTimestampFormatter, Day0ProducesNilvalue)
{
    ts.Day = 0;

    format();

    CHECK_FORMATTED("-");
}

TEST(SolidSyslogTimestampFormatter, Day32ProducesNilvalue)
{
    ts.Day = 32;

    format();

    CHECK_FORMATTED("-");
}

TEST(SolidSyslogTimestampFormatter, Hour0FormatsAsZeroPadded00)
{
    ts.Hour = 0;

    format();

    CHECK_FORMATTED("2026-04-02T00:30:07.000042Z");
}

TEST(SolidSyslogTimestampFormatter, Hour23FormatsAs23)
{
    ts.Hour = 23;

    format();

    CHECK_FORMATTED("2026-04-02T23:30:07.000042Z");
}

TEST(SolidSyslogTimestampFormatter, Hour24ProducesNilvalue)
{
    ts.Hour = 24;

    format();

    CHECK_FORMATTED("-");
}

TEST(SolidSyslogTimestampFormatter, Minute0FormatsAsZeroPadded00)
{
    ts.Minute = 0;

    format();

    CHECK_FORMATTED("2026-04-02T14:00:07.000042Z");
}

TEST(SolidSyslogTimestampFormatter, Minute59FormatsAs59)
{
    ts.Minute = 59;

    format();

    CHECK_FORMATTED("2026-04-02T14:59:07.000042Z");
}

TEST(SolidSyslogTimestampFormatter, Minute60ProducesNilvalue)
{
    ts.Minute = 60;

    format();

    CHECK_FORMATTED("-");
}

TEST(SolidSyslogTimestampFormatter, Second0FormatsAsZeroPadded00)
{
    ts.Second = 0;

    format();

    CHECK_FORMATTED("2026-04-02T14:30:00.000042Z");
}

TEST(SolidSyslogTimestampFormatter, Second59FormatsAs59)
{
    ts.Second = 59;

    format();

    CHECK_FORMATTED("2026-04-02T14:30:59.000042Z");
}

TEST(SolidSyslogTimestampFormatter, Second60ProducesNilvalue)
{
    ts.Second = 60;

    format();

    CHECK_FORMATTED("-");
}

TEST(SolidSyslogTimestampFormatter, Microsecond0FormatsAsSixDigitZeroPadded000000)
{
    ts.Microsecond = 0;

    format();

    CHECK_FORMATTED("2026-04-02T14:30:07.000000Z");
}

TEST(SolidSyslogTimestampFormatter, Microsecond999999FormatsAs999999)
{
    ts.Microsecond = 999999;

    format();

    CHECK_FORMATTED("2026-04-02T14:30:07.999999Z");
}

TEST(SolidSyslogTimestampFormatter, Microsecond1000000ProducesNilvalue)
{
    ts.Microsecond = 1000000;

    format();

    CHECK_FORMATTED("-");
}

TEST(SolidSyslogTimestampFormatter, ZeroOffsetFormatsAsZ)
{
    ts.UtcOffsetMinutes = 0;

    format();

    CHECK_FORMATTED("2026-04-02T14:30:07.000042Z");
}

TEST(SolidSyslogTimestampFormatter, PositiveOffsetFormatsAsPlusHHMM)
{
    ts.UtcOffsetMinutes = 330;

    format();

    CHECK_FORMATTED("2026-04-02T14:30:07.000042+05:30");
}

TEST(SolidSyslogTimestampFormatter, NegativeOffsetFormatsAsMinusHHMM)
{
    ts.UtcOffsetMinutes = -300;

    format();

    CHECK_FORMATTED("2026-04-02T14:30:07.000042-05:00");
}

TEST(SolidSyslogTimestampFormatter, UtcOffsetPlus840FormatsAsPlus1400)
{
    ts.UtcOffsetMinutes = 840;

    format();

    CHECK_FORMATTED("2026-04-02T14:30:07.000042+14:00");
}

TEST(SolidSyslogTimestampFormatter, UtcOffsetMinus720FormatsAsMinus1200)
{
    ts.UtcOffsetMinutes = -720;

    format();

    CHECK_FORMATTED("2026-04-02T14:30:07.000042-12:00");
}

TEST(SolidSyslogTimestampFormatter, UtcOffsetPlus841ProducesNilvalue)
{
    ts.UtcOffsetMinutes = 841;

    format();

    CHECK_FORMATTED("-");
}

TEST(SolidSyslogTimestampFormatter, UtcOffsetMinus721ProducesNilvalue)
{
    ts.UtcOffsetMinutes = -721;

    format();

    CHECK_FORMATTED("-");
}
