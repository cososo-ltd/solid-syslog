#ifndef SOLIDSYSLOGPOSIXSLEEP_H
#define SOLIDSYSLOGPOSIXSLEEP_H

#include "ExternC.h"

EXTERN_C_BEGIN

    /* POSIX implementation of SolidSyslogSleepFunction. Wraps nanosleep so
       the TLS handshake retry loop yields to the kernel between attempts. */
    void SolidSyslogPosixSleep(int milliseconds);

EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXSLEEP_H */
