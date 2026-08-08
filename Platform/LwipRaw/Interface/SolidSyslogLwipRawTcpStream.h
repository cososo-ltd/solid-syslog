/** @file
 *  A TCP stream over the lwIP Raw API, for a StreamSender or as the byte
 *  transport under a TlsStream / MbedTlsStream.
 *
 *  Every lwIP call runs under the SolidSyslogLwipRaw_Marshal hop; the callbacks
 *  lwIP fires back (connected / recv / err) only flip flags, so they stay
 *  unmarshalled. What the stream does through its vtable is the substance:
 *
 *  - Open runs tcp_new + pcb setup + tcp_connect in one marshalled batch, then
 *    spins on the caller's thread — sleeping via the config's Sleep so lwIP's
 *    timer / RX paths advance the SYN exchange — until the connected callback
 *    reports success, an error, or the connect deadline (config's
 *    GetConnectTimeoutMs, re-read each attempt so a runtime-tunable value applies
 *    on the next reconnect) elapses. On failure the half-open pcb is tcp_abort'd.
 *    Every pcb carries SOF_KEEPALIVE, and Nagle is off (TCP_NODELAY) so a small
 *    latency-sensitive record — or a stacked TLS handshake flight — is not held
 *    for an ACK.
 *  - Send is all-or-nothing: tcp_write uses TCP_WRITE_FLAG_COPY, so the caller's
 *    buffer lifetime ends at return; any write/output failure closes the stream
 *    for the sender to reconnect. A peer FIN marks the stream unwritable while
 *    still readable, so buffered bytes drain before the close.
 *  - Read drains the bounded RX pbuf queue (size
 *    SOLIDSYSLOG_LWIP_RAW_TCP_RX_QUEUE_SIZE; a full queue backpressures lwIP,
 *    which replays the pbuf later), returning bytes copied, 0 for would-block, or
 *    < 0 once a drained peer FIN closes the stream internally.
 *
 *  It encapsulates lwIP's tcp_close-after-tcp_err use-after-free rule: the err
 *  callback nulls the pcb pointer, and Close only calls tcp_close when the
 *  pointer is still live, so a released pcb is never closed twice. Accepted
 *  pbufs are always freed on close regardless of pcb state. See
 *  docs/platforms/lwipraw/setup.md for the full integrator guide.
 *
 *  @ingroup platform_lwipraw */
#ifndef SOLIDSYSLOGLWIPRAWTCPSTREAM_H
#define SOLIDSYSLOGLWIPRAWTCPSTREAM_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogSleep.h"
#include "SolidSyslogTcpConnectTimeoutFunction.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogStream;

    /** Tunes SolidSyslogLwipRawTcpStream's bounded connect. */
    struct SolidSyslogLwipRawTcpStreamConfig
    {
        /** Per-attempt connect deadline in ms; NULL uses the
         *  SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS tunable. */
        SolidSyslogTcpConnectTimeoutFunction GetConnectTimeoutMs;
        void* ConnectTimeoutContext; /**< Passed back to GetConnectTimeoutMs unchanged; NULL is fine. */
        /** Required; each connect-spin iteration sleeps through it so lwIP can
         *  advance the handshake. A NULL config or NULL Sleep falls back to NullStream. */
        SolidSyslogSleepFunction Sleep;
    };

    /** Draw a TCP stream from the pool; the config's Sleep drives the bounded
     *  connect and its GetConnectTimeoutMs bounds it (see the file overview for
     *  the stream's behaviour). A NULL config, a NULL Sleep, or an exhausted pool
     *  (default size 2, for the TLS-over-plain-TCP pair) falls back to the shared
     *  NullStream. */
    struct SolidSyslogStream* SolidSyslogLwipRawTcpStream_Create(const struct SolidSyslogLwipRawTcpStreamConfig* config
    );
    /** Release the pool slot and close the connection. */
    void SolidSyslogLwipRawTcpStream_Destroy(struct SolidSyslogStream * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWTCPSTREAM_H */
