/** @file
 *  The Windows SolidSyslogSleepFunction. */
#ifndef SOLIDSYSLOGWINDOWSSLEEP_H
#define SOLIDSYSLOGWINDOWSSLEEP_H

#include "ExternC.h"

EXTERN_C_BEGIN

    /** Wraps Sleep so a bounded retry loop (e.g. the TLS handshake) yields to the
     *  scheduler between attempts. */
    void SolidSyslogWindowsSleep(int milliseconds);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSSLEEP_H */
