/** @file
 *  The POSIX SolidSyslogSleepFunction.
 *
 *  @ingroup platform_posix */
#ifndef SOLIDSYSLOGPOSIXSLEEP_H
#define SOLIDSYSLOGPOSIXSLEEP_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Sleeps for @p milliseconds via nanosleep. It neither performs nor bounds
     *  retries; callers such as the TLS handshake use it to yield between their own
     *  bounded attempts. */
    void SolidSyslogPosixSleep(int milliseconds);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXSLEEP_H */
