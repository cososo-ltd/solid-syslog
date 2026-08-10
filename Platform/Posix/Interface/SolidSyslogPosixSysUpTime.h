/** @file
 *  The POSIX SolidSyslogSysUpTimeFunction, for MetaSd. */
#ifndef SOLIDSYSLOGPOSIXSYSUPTIME_H
#define SOLIDSYSLOGPOSIXSYSUPTIME_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Hundredths of a second since boot from CLOCK_BOOTTIME, as RFC 3418
     *  sysUpTime; wraps modulo 2^32 per the TimeTicks contract. Returns 0 if the
     *  clock read fails. */
    uint32_t SolidSyslogPosix_GetSysUpTime(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXSYSUPTIME_H */
