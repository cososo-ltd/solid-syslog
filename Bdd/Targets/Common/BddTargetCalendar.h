#ifndef BDDTARGETCALENDAR_H
#define BDDTARGETCALENDAR_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogTimestamp.h"

#include <stdint.h>

/* Converts seconds since the Unix epoch to a date and time of day in UTC, and
   back. The seconds are 32 bits, so the range is 1970 to early 2106. */

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Fills in the date and the time of day, and leaves the microseconds and
       the offset from UTC at zero. */
    void BddTargetCalendar_FromEpoch(uint32_t secondsSinceEpoch, struct SolidSyslogTimestamp* timestamp);

    /* The date and the time of day are taken as UTC whatever the offset says,
       and the microseconds are dropped. A date outside the 32-bit range is not
       checked for. */
    uint32_t BddTargetCalendar_ToEpoch(const struct SolidSyslogTimestamp* timestamp);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETCALENDAR_H */
