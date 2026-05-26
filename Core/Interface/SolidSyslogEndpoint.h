#ifndef SOLIDSYSLOGENDPOINT_H
#define SOLIDSYSLOGENDPOINT_H

#include <stdint.h>

#include "ExternC.h"

EXTERN_C_BEGIN

    enum
    {
        SOLIDSYSLOG_MAX_HOST_SIZE = 256
    };

    struct SolidSyslogEndpoint
    {
        struct SolidSyslogFormatter* Host; /* library-provided; user writes destination host into it */
        uint16_t Port; /* user assigns destination port */
    };

    typedef void (*SolidSyslogEndpointFunction)(struct SolidSyslogEndpoint* endpoint);

    /* Cheap pure function returning a monotonic version counter the user bumps on any change to host or port.
       The sender polls this on every Send and only re-pulls the endpoint when the version differs from the
       last value it acted on. */
    typedef uint32_t (*SolidSyslogEndpointVersionFunction)(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGENDPOINT_H */
