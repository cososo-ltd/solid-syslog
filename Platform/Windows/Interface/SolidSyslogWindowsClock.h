/** @file
 *  The Windows SolidSyslogClockFunction, for SolidSyslogConfig.Clock. */
#ifndef SOLIDSYSLOGWINDOWSCLOCK_H
#define SOLIDSYSLOGWINDOWSCLOCK_H

#include "SolidSyslogTimestamp.h"

EXTERN_C_BEGIN

    /** Fills @p timestamp from the system wall clock (GetSystemTimeAsFileTime),
     *  broken down to UTC calendar fields with microsecond precision. */
    void SolidSyslogWindowsClock_GetTimestamp(struct SolidSyslogTimestamp * timestamp);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSCLOCK_H */
