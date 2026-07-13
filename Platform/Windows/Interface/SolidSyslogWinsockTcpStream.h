/** @file
 *  A non-blocking TCP stream over a Winsock socket, for a StreamSender or as the
 *  byte transport under a TlsStream.
 *
 *  What the stream does through its vtable is the substance:
 *
 *  - Open makes the socket non-blocking (FIONBIO) before connecting, so connect
 *    returns WSAEWOULDBLOCK and the wait is bounded by a select() up to the
 *    config's GetConnectTimeoutMs (re-read each attempt, so a runtime-tunable
 *    value applies on the next reconnect); a refused or unreachable peer fails
 *    fast. This deliberately sidesteps Winsock's blocking connect(), which
 *    retries a refused loopback port internally for ~2 s before returning.
 *  - Send is all-or-nothing and never blocks the service thread: a short write
 *    or any error is taken as a dead connection, so the stream closes itself and
 *    the sender reconnects on its next pass.
 *  - Read returns the bytes read, 0 for would-block (WSAEWOULDBLOCK, connection
 *    kept), or tears the connection down on peer close or error.
 *
 *  TCP_NODELAY is on, and kernel keepalive (idle ~45s, then 4 x 10s probes via
 *  TCP_KEEPIDLE / TCP_KEEPINTVL / TCP_KEEPCNT) surfaces a wedged peer as a failed
 *  Send/Read. Windows has no TCP_USER_TIMEOUT analogue, so a pending unacked
 *  write relies on the OS-default retransmit timeout.
 *
 *  The caller must invoke WSAStartup before use and WSACleanup on shutdown; the
 *  library does not manage the Winsock lifecycle. */
#ifndef SOLIDSYSLOGWINSOCKTCPSTREAM_H
#define SOLIDSYSLOGWINSOCKTCPSTREAM_H

#include "SolidSyslogStream.h"
#include "SolidSyslogTcpConnectTimeoutFunction.h"
#include "SolidSyslogTransport.h"

EXTERN_C_BEGIN

    /** Tunes SolidSyslogWinsockTcpStream's bounded connect. */
    struct SolidSyslogWinsockTcpStreamConfig
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
    struct SolidSyslogStream* SolidSyslogWinsockTcpStream_Create(const struct SolidSyslogWinsockTcpStreamConfig* config
    );
    /** Release the pool slot and close the socket. */
    void SolidSyslogWinsockTcpStream_Destroy(struct SolidSyslogStream * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKTCPSTREAM_H */
