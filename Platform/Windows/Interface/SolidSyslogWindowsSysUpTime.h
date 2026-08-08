/** @file
 *  The Windows SolidSyslogSysUpTimeFunction, for MetaSd.
 *
 *  @ingroup platform_windows */
#ifndef SOLIDSYSLOGWINDOWSSYSUPTIME_H
#define SOLIDSYSLOGWINDOWSSYSUPTIME_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Hundredths of a second since boot from GetTickCount64, as RFC 3418
     *  sysUpTime; wraps modulo 2^32 per the TimeTicks contract. */
    uint32_t SolidSyslogWindowsSysUpTime_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSSYSUPTIME_H */
