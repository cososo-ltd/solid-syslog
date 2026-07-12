#ifndef SOLIDSYSLOGSLEEP_H
#define SOLIDSYSLOGSLEEP_H

#include "ExternC.h"

EXTERN_C_BEGIN

    /** Millisecond sleep callback injected via config. Drives the bounded
     *  wait-and-retry loops that would otherwise busy-spin: the TLS handshake
     *  poll, and the lwIP TCP connect and DNS resolve spins. Injecting it keeps
     *  platform sleep APIs out of the library and lets tests pass a no-op with
     *  no conditional compilation. */
    typedef void (*SolidSyslogSleepFunction)(int milliseconds);

EXTERN_C_END

#endif /* SOLIDSYSLOGSLEEP_H */
