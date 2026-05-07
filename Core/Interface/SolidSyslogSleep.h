#ifndef SOLIDSYSLOGSLEEP_H
#define SOLIDSYSLOGSLEEP_H

#include "ExternC.h"

EXTERN_C_BEGIN

    /* Cross-platform millisecond sleep callback. The TLS handshake retry loop
       (and any future wait-and-retry path) takes one of these via config so
       the library never sees platform-specific sleep APIs and tests can
       inject a no-op without conditional compilation. */
    typedef void (*SolidSyslogSleepFunction)(int milliseconds);

EXTERN_C_END

#endif /* SOLIDSYSLOGSLEEP_H */
