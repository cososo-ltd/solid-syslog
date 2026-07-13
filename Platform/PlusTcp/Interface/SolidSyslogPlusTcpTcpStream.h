/** @file
 *  A TCP stream over a FreeRTOS-Plus-TCP socket, for a StreamSender or as the
 *  byte transport under a TlsStream.
 *
 *  What the stream does through its vtable is the substance:
 *
 *  - Open connects with a bounded blocking wait: on an ARP-cache miss it first
 *    issues an ARP probe and yields once (~50 ms) so the cold-start SYN is not
 *    dropped at the IP layer, then sets SO_SNDTIMEO and SO_RCVTIMEO to the
 *    config's GetConnectTimeoutMs (re-read each attempt, so a runtime-tunable
 *    value applies on the next reconnect) and calls FreeRTOS_connect. On success
 *    both timeouts are cleared to 0 so subsequent Send/Read are non-blocking
 *    single calls; a failed connect closes the socket and fails fast.
 *  - Send is all-or-nothing and never blocks the service thread: a short write
 *    or any error is taken as a dead connection, so the stream closes itself and
 *    the sender reconnects on its next pass.
 *  - Read returns the bytes read, 0 for would-block (RCVTIMEO=0, connection
 *    kept), or tears the connection down on error. */
#ifndef SOLIDSYSLOGPLUSTCPTCPSTREAM_H
#define SOLIDSYSLOGPLUSTCPTCPSTREAM_H

#include "ExternC.h"
#include "SolidSyslogTcpConnectTimeoutFunction.h"

EXTERN_C_BEGIN

    struct SolidSyslogStream;

    /** Tunes SolidSyslogPlusTcpTcpStream's bounded connect. */
    struct SolidSyslogPlusTcpTcpStreamConfig
    {
        /** Per-attempt connect deadline in ms; NULL uses the
         *  SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS tunable. */
        SolidSyslogTcpConnectTimeoutFunction GetConnectTimeoutMs;
        void* ConnectTimeoutContext; /**< Passed back to GetConnectTimeoutMs unchanged; NULL is fine. */
    };

    /** Draw a TCP stream from the pool; the config's GetConnectTimeoutMs bounds
     *  the connect (see the file overview for the stream's behaviour). An
     *  exhausted pool (default size 2, for the plain-TCP + TLS-under-TCP pair)
     *  falls back to the shared NullStream. */
    struct SolidSyslogStream* SolidSyslogPlusTcpTcpStream_Create(const struct SolidSyslogPlusTcpTcpStreamConfig* config
    );
    /** Release the pool slot and close the socket. */
    void SolidSyslogPlusTcpTcpStream_Destroy(struct SolidSyslogStream * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSTCPTCPSTREAM_H */
