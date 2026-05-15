#ifndef SOLIDSYSLOGTIMESTAMP_H
#define SOLIDSYSLOGTIMESTAMP_H

#include "ExternC.h"

#include <stdint.h>

EXTERN_C_BEGIN

    /* Broken-down timestamp as emitted by a SolidSyslogClockFunction.
       All calendar/time fields are expressed in the timezone indicated by
       utcOffsetMinutes — the built-in POSIX and Windows clocks both produce
       UTC (utcOffsetMinutes == 0).

       A clock may leave the struct zeroed to signal "no usable timestamp".
       The library validates each field before formatting; the ranges below
       are the ones enforced by the internal TimestampIsValid check. A
       zeroed struct fails validation because month == 0 is out of range. */
    struct SolidSyslogTimestamp
    {
        uint16_t
            Year; /* Gregorian year, e.g. 2026. Not independently validated — clocks are trusted to produce a sensible value. */
        uint8_t Month; /* 1-12. */
        uint8_t Day; /* 1-31. */
        uint8_t Hour; /* 0-23. */
        uint8_t Minute; /* 0-59. */
        uint8_t Second; /* 0-59. Leap seconds are not represented. */
        uint32_t Microsecond; /* 0-999999. */
        int16_t
            UtcOffsetMinutes; /* Offset from UTC in minutes; 0 for UTC. Must be -720..840 (UTC-12:00 to UTC+14:00). */
    };

    typedef void (*SolidSyslogClockFunction)(struct SolidSyslogTimestamp* timestamp);

EXTERN_C_END

#endif /* SOLIDSYSLOGTIMESTAMP_H */
