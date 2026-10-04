#ifndef VXWORKS64CLOCKFAKE_H
#define VXWORKS64CLOCKFAKE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Controls the time calls the stand-in time.h renames, in plain integers,
     * so a test never includes that header. */
    void VxWorks64ClockFake_Reset(void);

    /** Make clock_gettime answer ERROR. */
    void VxWorks64ClockFake_FailClockGettime(void);

    /** Make gmtime_r answer ERROR. */
    void VxWorks64ClockFake_FailGmtimeR(void);

    /** What gmtime_r answers, as raw struct tm fields: years since 1900 and a
     *  zero-based month. */
    void VxWorks64ClockFake_SetBrokenDownTime(int year, int month, int day, int hour, int minute, int second);

    /** What clock_gettime answers in tv_nsec. */
    void VxWorks64ClockFake_SetNanoseconds(long nanoseconds);

SOLIDSYSLOG_EXTERN_C_END

#endif /* VXWORKS64CLOCKFAKE_H */
