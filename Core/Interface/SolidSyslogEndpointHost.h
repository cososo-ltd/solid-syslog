#ifndef SOLIDSYSLOGENDPOINTHOST_H
#define SOLIDSYSLOGENDPOINTHOST_H

#include "ExternC.h"

#include <stddef.h>

EXTERN_C_BEGIN

    /* The value sink for a destination host (the SolidSyslogEndpoint a sender
     * hands its endpoint callback). A callback is given only a
     * SolidSyslogEndpointHost* — it can append the host string bounded to the
     * host-field width, but cannot reach the raw formatter. Unlike a header
     * field, the bytes are copied verbatim: a DNS name or IP literal headed to
     * the resolver must not be silently substituted. Stack-transient, no pool
     * (D.002). */
    struct SolidSyslogEndpointHost;

    /* Appends up to maxLength bytes of source (stopping at a NUL terminator)
     * verbatim into the host sink, further bounded by the host-field width the
     * sink was created with. */
    void SolidSyslogEndpointHost_String(struct SolidSyslogEndpointHost * host, const char* source, size_t maxLength);

EXTERN_C_END

#endif /* SOLIDSYSLOGENDPOINTHOST_H */
