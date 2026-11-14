#ifndef BDDTARGETCALENDAR_H
#define BDDTARGETCALENDAR_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogTimestamp.h"

#include <stdint.h>

/* Seconds since the Unix epoch as a date and a time of day in UTC, and back.
   For a target whose clock counts one and whose hardware or whose library
   wants the other. Thirty-two bits of seconds, so 1970 to early 2106. */

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Fills in the date and the time of day, and leaves the microseconds and
       the offset from UTC at zero. */
    void BddTargetCalendar_FromEpoch(uint32_t secondsSinceEpoch, struct SolidSyslogTimestamp* timestamp);

    /* The date and the time of day are taken as UTC whatever the offset says,
       and the microseconds are dropped. A date outside what thirty-two bits of
       seconds reach has no answer worth having, and is not looked for. */
    uint32_t BddTargetCalendar_ToEpoch(const struct SolidSyslogTimestamp* timestamp);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETCALENDAR_H */
