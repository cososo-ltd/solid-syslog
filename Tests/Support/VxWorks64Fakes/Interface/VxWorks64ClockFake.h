#ifndef VXWORKS64CLOCKFAKE_H
#define VXWORKS64CLOCKFAKE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Controls the time calls the stand-in time.h renames, in plain integers,
     * so a test never includes that header. */
    void VxWorks64ClockFake_Reset(void);

    /** Make clock_gettime answer ERROR. */
    void VxWorks64ClockFake_FailClockGettime(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* VXWORKS64CLOCKFAKE_H */
