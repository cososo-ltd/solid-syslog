/** @file
 *  A Sender that delivers each message octet-framed (RFC 6587 octet-counting: a
 *  decimal length, a space, then the message bytes) over any injected Stream —
 *  plain TCP, TLS, or a caller-supplied byte transport. It resolves the endpoint
 *  and opens the stream lazily on the first Send, reconnecting when the endpoint
 *  version changes or after any send failure (a short or failed write closes the
 *  stream so the next Send reconnects). Repeated delivery failures and the
 *  recovery after them are reported once each via SolidSyslog_Error
 *  (DELIVERY_FAILED / DELIVERY_RESTORED), so a flapping link is not a log storm.
 *  Destroy closes the stream but does not free the injected Resolver, Stream, or
 *  Address. */
#ifndef SOLIDSYSLOG_STREAM_SENDER_H
#define SOLIDSYSLOG_STREAM_SENDER_H

#include "SolidSyslogEndpoint.h"
#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogSender;

    /** Wiring for SolidSyslogStreamSender_Create, a sender that octet-frames each
     *  message (RFC 6587) over any injected Stream (plain TCP, TLS, or a
     *  caller-supplied byte transport). Resolver, Stream, and Address are required
     *  (NULL makes Create fall back to the shared NullSender); the rest is
     *  optional. Everything you inject must outlive the created handle, but the
     *  config struct itself is copied at Create and may be transient. */
    struct SolidSyslogStreamSenderConfig
    {
        struct SolidSyslogResolver* Resolver;
        struct SolidSyslogStream* Stream;
        /** Destination slot the Resolver writes and the Stream opens against; one per sender. */
        struct SolidSyslogAddress* Address;
        /** Fills host/port; called only on (re)connect. NULL resolves an empty host on port 0. */
        SolidSyslogEndpointFunction Endpoint;
        /** Polled every Send to detect an endpoint change; NULL pins the destination (never reconnects for a change). */
        SolidSyslogEndpointVersionFunction EndpointVersion;
        void* EndpointContext; /**< Passed to Endpoint and EndpointVersion unchanged. */
    };

    /** Create a stream sender from @p config. Never returns NULL: a NULL or
     *  invalid config, or an exhausted sender pool, reports via SolidSyslog_Error
     *  and returns the shared NullSender (Send drops on the floor), so the result
     *  is safe to wire without a null-check. */
    struct SolidSyslogSender* SolidSyslogStreamSender_Create(const struct SolidSyslogStreamSenderConfig* config);

    /** Release the sender's pool slot, closing the stream first. Does not free the
     *  injected Resolver, Stream, or Address; the caller owns those. */
    void SolidSyslogStreamSender_Destroy(struct SolidSyslogSender * base);

EXTERN_C_END

#endif /* SOLIDSYSLOG_STREAM_SENDER_H */
