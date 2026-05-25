#ifndef SOLIDSYSLOGTCPCONNECTTIMEOUTFUNCTION_H
#define SOLIDSYSLOGTCPCONNECTTIMEOUTFUNCTION_H

#include <stdint.h>

#include "ExternC.h"

EXTERN_C_BEGIN

    /* Returns the bounded-connect deadline in milliseconds. Called at the
       start of every connect attempt so a runtime-tunable value (e.g.
       commissioning-time NVRAM read, live web-UI override) takes effect on
       the next reconnect without rebuilding or destroying the stream.
       The void* context is passed through unchanged from the config slot
       the integrator installed it on. */
    typedef uint32_t (*SolidSyslogTcpConnectTimeoutFunction)(void* context);

EXTERN_C_END

#endif /* SOLIDSYSLOGTCPCONNECTTIMEOUTFUNCTION_H */
