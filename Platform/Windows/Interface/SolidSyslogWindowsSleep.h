#ifndef SOLIDSYSLOGWINDOWSSLEEP_H
#define SOLIDSYSLOGWINDOWSSLEEP_H

#include "ExternC.h"

EXTERN_C_BEGIN

    /* Windows implementation of SolidSyslogSleepFunction. Wraps Sleep so the
       TLS handshake retry loop yields to the scheduler between attempts. */
    void SolidSyslogWindowsSleep(int milliseconds);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSSLEEP_H */
