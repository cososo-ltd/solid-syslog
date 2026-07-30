/** @file
 *  The Windows SolidSyslogSleepFunction. */
#ifndef SOLIDSYSLOGWINDOWSSLEEP_H
#define SOLIDSYSLOGWINDOWSSLEEP_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Wraps Sleep so a bounded retry loop (e.g. the TLS handshake) yields to the
     *  scheduler between attempts. */
    void SolidSyslogWindowsSleep(int milliseconds);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSSLEEP_H */
