#include "SolidSyslogTimestampFormatter.h"

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogTimestamp.h"

struct SolidSyslogFormatter;

static inline bool TimestampFormatter_IsValid(const struct SolidSyslogTimestamp* ts);
static inline void TimestampFormatter_FormatValid(
    struct SolidSyslogFormatter* f,
    const struct SolidSyslogTimestamp* ts
);
static inline void TimestampFormatter_FormatUtcOffset(struct SolidSyslogFormatter* f, int16_t offsetMinutes);
static inline void TimestampFormatter_FormatNonZeroUtcOffset(struct SolidSyslogFormatter* f, int16_t offsetMinutes);
static inline int16_t TimestampFormatter_AbsoluteInt16(int16_t value);

void SolidSyslogTimestampFormatter_Format(
    struct SolidSyslogFormatter* formatter,
    const struct SolidSyslogTimestamp* timestamp
)
{
    if (TimestampFormatter_IsValid(timestamp))
    {
        TimestampFormatter_FormatValid(formatter, timestamp);
    }
    else
    {
        SolidSyslogFormatter_NilValue(formatter);
    }
}

static inline bool TimestampFormatter_IsValid(const struct SolidSyslogTimestamp* ts)
{
    bool valid = true;

    valid = valid && (ts->Month >= 1U) && (ts->Month <= 12U);
    valid = valid && (ts->Day >= 1U) && (ts->Day <= 31U);
    valid = valid && (ts->Hour <= 23U);
    valid = valid && (ts->Minute <= 59U);
    valid = valid && (ts->Second <= 59U);
    valid = valid && (ts->Microsecond <= 999999U);
    valid = valid && (ts->UtcOffsetMinutes >= -720) && (ts->UtcOffsetMinutes <= 840);

    return valid;
}

static inline void TimestampFormatter_FormatValid(struct SolidSyslogFormatter* f, const struct SolidSyslogTimestamp* ts)
{
    SolidSyslogFormatter_FourDigit(f, ts->Year);
    SolidSyslogFormatter_AsciiCharacter(f, '-');
    SolidSyslogFormatter_TwoDigit(f, ts->Month);
    SolidSyslogFormatter_AsciiCharacter(f, '-');
    SolidSyslogFormatter_TwoDigit(f, ts->Day);
    SolidSyslogFormatter_AsciiCharacter(f, 'T');
    SolidSyslogFormatter_TwoDigit(f, ts->Hour);
    SolidSyslogFormatter_AsciiCharacter(f, ':');
    SolidSyslogFormatter_TwoDigit(f, ts->Minute);
    SolidSyslogFormatter_AsciiCharacter(f, ':');
    SolidSyslogFormatter_TwoDigit(f, ts->Second);
    SolidSyslogFormatter_AsciiCharacter(f, '.');
    SolidSyslogFormatter_SixDigit(f, ts->Microsecond);
    TimestampFormatter_FormatUtcOffset(f, ts->UtcOffsetMinutes);
}

static inline void TimestampFormatter_FormatUtcOffset(struct SolidSyslogFormatter* f, int16_t offsetMinutes)
{
    if (offsetMinutes == 0)
    {
        SolidSyslogFormatter_AsciiCharacter(f, 'Z');
    }
    else
    {
        TimestampFormatter_FormatNonZeroUtcOffset(f, offsetMinutes);
    }
}

static inline void TimestampFormatter_FormatNonZeroUtcOffset(struct SolidSyslogFormatter* f, int16_t offsetMinutes)
{
    uint32_t absoluteMinutes = (uint32_t) TimestampFormatter_AbsoluteInt16(offsetMinutes);

    SolidSyslogFormatter_AsciiCharacter(f, (offsetMinutes > 0) ? '+' : '-');
    SolidSyslogFormatter_TwoDigit(f, absoluteMinutes / 60U);
    SolidSyslogFormatter_AsciiCharacter(f, ':');
    SolidSyslogFormatter_TwoDigit(f, absoluteMinutes % 60U);
}

static inline int16_t TimestampFormatter_AbsoluteInt16(int16_t value)
{
    int16_t result = value;

    if (value < 0)
    {
        result = (int16_t) (-value);
    }

    return result;
}
