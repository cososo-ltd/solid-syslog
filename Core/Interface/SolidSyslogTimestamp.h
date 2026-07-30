/** @file
 *  The broken-down timestamp struct and the SolidSyslogClockFunction that fills
 *  it, supplying the RFC 5424 TIMESTAMP field. */
#ifndef SOLIDSYSLOGTIMESTAMP_H
#define SOLIDSYSLOGTIMESTAMP_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Broken-down timestamp as emitted by a SolidSyslogClockFunction. All
     *  calendar/time fields are expressed in the timezone indicated by
     *  UtcOffsetMinutes; the built-in POSIX and Windows clocks both produce UTC
     *  (UtcOffsetMinutes == 0).
     *
     *  A clock may leave the struct zeroed to signal "no usable timestamp": the
     *  library validates each field before formatting, and a zeroed struct fails
     *  validation because Month == 0 is out of range. */
    struct SolidSyslogTimestamp
    {
        uint16_t Year; /**< Gregorian year, e.g. 2026. Not range-checked; clocks are trusted here. */
        uint8_t Month; /**< 1-12. */
        uint8_t Day; /**< 1-31. */
        uint8_t Hour; /**< 0-23. */
        uint8_t Minute; /**< 0-59. */
        uint8_t Second; /**< 0-59. Leap seconds are not represented. */
        uint32_t Microsecond; /**< 0-999999. */
        int16_t UtcOffsetMinutes; /**< Minutes from UTC; 0 for UTC. Valid -720..840 (UTC-12:00 to UTC+14:00). */
    };

    /** Fills @p timestamp with the current time, or zeroes it to signal "no
     *  usable timestamp". Installed as SolidSyslogConfig.Clock. */
    typedef void (*SolidSyslogClockFunction)(struct SolidSyslogTimestamp* timestamp);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGTIMESTAMP_H */
