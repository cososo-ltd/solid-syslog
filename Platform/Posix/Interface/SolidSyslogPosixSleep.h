/** @file
 *  The POSIX SolidSyslogSleepFunction. */
#ifndef SOLIDSYSLOGPOSIXSLEEP_H
#define SOLIDSYSLOGPOSIXSLEEP_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Sleeps for @p milliseconds via nanosleep. It neither performs nor bounds
     *  retries; callers such as the TLS handshake use it to yield between their own
     *  bounded attempts. */
    void SolidSyslogPosix_Sleep(int milliseconds);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXSLEEP_H */
