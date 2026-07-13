/** @file
 *  The Windows SolidSyslogSysUpTimeFunction, for MetaSd. */
#ifndef SOLIDSYSLOGWINDOWSSYSUPTIME_H
#define SOLIDSYSLOGWINDOWSSYSUPTIME_H

#include "ExternC.h"

#include <stdint.h>

EXTERN_C_BEGIN

    /** Hundredths of a second since boot from GetTickCount64, as RFC 3418
     *  sysUpTime; wraps modulo 2^32 per the TimeTicks contract. */
    uint32_t SolidSyslogWindowsSysUpTime_Get(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSSYSUPTIME_H */
