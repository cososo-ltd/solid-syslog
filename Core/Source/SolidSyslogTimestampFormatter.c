/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogTimestampFormatter.h"

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogTimestamp.h"

struct SolidSyslogFormatter;

static inline bool TimestampFormatter_IsValid(const struct SolidSyslogTimestamp* ts);
static inline void TimestampFormatter_FormatValid(
    struct SolidSyslogFormatter* formatter,
    const struct SolidSyslogTimestamp* ts
);
static inline void TimestampFormatter_FormatUtcOffset(struct SolidSyslogFormatter* formatter, int16_t offsetMinutes);
static inline void TimestampFormatter_FormatNonZeroUtcOffset(
    struct SolidSyslogFormatter* formatter,
    int16_t offsetMinutes
);
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

static inline void TimestampFormatter_FormatValid(
    struct SolidSyslogFormatter* formatter,
    const struct SolidSyslogTimestamp* ts
)
{
    SolidSyslogFormatter_FourDigit(formatter, ts->Year);
    SolidSyslogFormatter_AsciiCharacter(formatter, '-');
    SolidSyslogFormatter_TwoDigit(formatter, ts->Month);
    SolidSyslogFormatter_AsciiCharacter(formatter, '-');
    SolidSyslogFormatter_TwoDigit(formatter, ts->Day);
    SolidSyslogFormatter_AsciiCharacter(formatter, 'T');
    SolidSyslogFormatter_TwoDigit(formatter, ts->Hour);
    SolidSyslogFormatter_AsciiCharacter(formatter, ':');
    SolidSyslogFormatter_TwoDigit(formatter, ts->Minute);
    SolidSyslogFormatter_AsciiCharacter(formatter, ':');
    SolidSyslogFormatter_TwoDigit(formatter, ts->Second);
    SolidSyslogFormatter_AsciiCharacter(formatter, '.');
    SolidSyslogFormatter_SixDigit(formatter, ts->Microsecond);
    TimestampFormatter_FormatUtcOffset(formatter, ts->UtcOffsetMinutes);
}

static inline void TimestampFormatter_FormatUtcOffset(struct SolidSyslogFormatter* formatter, int16_t offsetMinutes)
{
    if (offsetMinutes == 0)
    {
        SolidSyslogFormatter_AsciiCharacter(formatter, 'Z');
    }
    else
    {
        TimestampFormatter_FormatNonZeroUtcOffset(formatter, offsetMinutes);
    }
}

static inline void TimestampFormatter_FormatNonZeroUtcOffset(
    struct SolidSyslogFormatter* formatter,
    int16_t offsetMinutes
)
{
    uint32_t absoluteMinutes = (uint32_t) TimestampFormatter_AbsoluteInt16(offsetMinutes);

    SolidSyslogFormatter_AsciiCharacter(formatter, (offsetMinutes > 0) ? '+' : '-');
    SolidSyslogFormatter_TwoDigit(formatter, absoluteMinutes / 60U);
    SolidSyslogFormatter_AsciiCharacter(formatter, ':');
    SolidSyslogFormatter_TwoDigit(formatter, absoluteMinutes % 60U);
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
