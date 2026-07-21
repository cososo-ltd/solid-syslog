/** @file
 *  A Sender that transmits each message as a single UDP datagram over the
 *  injected Datagram, resolving the endpoint and opening the socket lazily on
 *  the first Send and re-resolving when the endpoint version changes. A datagram
 *  the path reports as oversize is retried once, trimmed to the datagram's
 *  MaxPayload on a UTF-8 codepoint boundary rather than dropped; an
 *  untransmittable record (trims to nothing, or still oversize on retry) is
 *  swallowed as sent so Service does not loop on it. A NULL send buffer is
 *  reported and rejected without touching delivery health. Repeated delivery
 *  failures and the recovery after them are reported once each via
 *  SolidSyslog_Error (DELIVERY_FAILED / DELIVERY_RESTORED). Destroy disconnects
 *  but does not free the injected Resolver, Datagram, or Address. */
#ifndef SOLIDSYSLOGUDPSENDER_H
#define SOLIDSYSLOGUDPSENDER_H

#include "SolidSyslogEndpoint.h"
#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogSender;

    /** Wiring for SolidSyslogUdpSender_Create, a sender that resolves a host and
     *  transmits each framed message as one datagram. Resolver, Datagram, Address,
     *  and Endpoint are required (NULL makes Create fall back to the shared
     *  NullSender); the rest is optional. Everything you inject must outlive the
     *  created handle, but the config struct itself is copied at Create and may
     *  be transient. */
    struct SolidSyslogUdpSenderConfig
    {
        struct SolidSyslogResolver* Resolver;
        struct SolidSyslogDatagram* Datagram;
        /** Destination slot the Resolver writes and the Datagram sends to; one per sender. */
        struct SolidSyslogAddress* Address;
        SolidSyslogEndpointFunction Endpoint; /**< Fills host/port; called only on (re)connect. */
        /** Polled every Send to detect an endpoint change; NULL pins the destination (never re-resolves). */
        SolidSyslogEndpointVersionFunction EndpointVersion;
        void* EndpointContext; /**< Passed to Endpoint and EndpointVersion unchanged. */
    };

    /** Create a UDP sender from @p config. Never returns NULL: a NULL or invalid
     *  config, or an exhausted sender pool, reports via SolidSyslog_Error and
     *  returns the shared NullSender (Send drops on the floor), so the result is
     *  safe to wire without a null-check. */
    struct SolidSyslogSender* SolidSyslogUdpSender_Create(const struct SolidSyslogUdpSenderConfig* config);

    /** Release the sender's pool slot, disconnecting first. Does not free the
     *  injected Resolver, Datagram, or Address; the caller owns those. */
    void SolidSyslogUdpSender_Destroy(struct SolidSyslogSender * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGUDPSENDER_H */
