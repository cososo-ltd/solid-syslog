#ifndef SOLIDSYSLOGENDPOINTHOST_H
#define SOLIDSYSLOGENDPOINTHOST_H

#include "ExternC.h"

#include <stddef.h>

EXTERN_C_BEGIN

    /** The value sink for a destination host (the SolidSyslogEndpoint.Host a
     *  sender hands its endpoint callback). The callback is given only a
     *  SolidSyslogEndpointHost*: it can append the host bounded to the host-field
     *  width, but cannot reach the raw formatter. Unlike a header field, the
     *  bytes are copied verbatim with no charset substitution, so a DNS name or
     *  IP literal reaches the resolver intact. Stack-transient, no pool (D.002). */
    struct SolidSyslogEndpointHost;

    /** Appends up to @p maxLength bytes of @p source (stopping at a NUL
     *  terminator) verbatim into @p host, further bounded by the host-field
     *  width the sink was created with. */
    void SolidSyslogEndpointHost_String(struct SolidSyslogEndpointHost * host, const char* source, size_t maxLength);

EXTERN_C_END

#endif /* SOLIDSYSLOGENDPOINTHOST_H */
