#ifndef SOLIDSYSLOGENDPOINTHOSTPRIVATE_H
#define SOLIDSYSLOGENDPOINTHOSTPRIVATE_H

#include "ExternC.h"

#include "SolidSyslogEndpointHost.h"

EXTERN_C_BEGIN

    struct SolidSyslogFormatter;

    /* Definition lives here (not the public header) so an endpoint callback
     * handed a SolidSyslogEndpointHost* cannot reach the wrapped formatter. */
    struct SolidSyslogEndpointHost
    {
        struct SolidSyslogFormatter* Formatter;
    };

    /* Internal constructor — wraps a sender's stack host formatter. The sender
     * builds one per resolve, passes it to the configured endpoint callback,
     * then reads the formatted host back out. Stack-transient: the caller owns
     * the storage. */
    void SolidSyslogEndpointHost_FromFormatter(
        struct SolidSyslogEndpointHost * host,
        struct SolidSyslogFormatter * formatter
    );

EXTERN_C_END

#endif /* SOLIDSYSLOGENDPOINTHOSTPRIVATE_H */
