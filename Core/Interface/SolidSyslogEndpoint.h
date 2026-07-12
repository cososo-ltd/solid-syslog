#ifndef SOLIDSYSLOGENDPOINT_H
#define SOLIDSYSLOGENDPOINT_H

#include <stdint.h>

#include "ExternC.h"

EXTERN_C_BEGIN

    enum
    {
        SOLIDSYSLOG_MAX_HOST_SIZE = 256 /**< Bound on the destination host a callback may write, in bytes. */
    };

    struct SolidSyslogEndpointHost;

    /** The destination a sender is directed at. The callback fills this in, not
     *  the sender: Host is a library-provided sink the user writes the host into
     *  (it cannot reach the raw formatter), Port the user assigns directly. */
    struct SolidSyslogEndpoint
    {
        struct SolidSyslogEndpointHost* Host;
        uint16_t Port;
    };

    /** Called by the sender to pull the current destination into @p endpoint. */
    typedef void (*SolidSyslogEndpointFunction)(struct SolidSyslogEndpoint* endpoint);

    /** Returns a monotonic version the user bumps on any change to host or port.
     *  The sender polls this every Send and re-pulls the endpoint only when the
     *  version differs from the last it acted on, so it must be cheap and pure. */
    typedef uint32_t (*SolidSyslogEndpointVersionFunction)(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGENDPOINT_H */
