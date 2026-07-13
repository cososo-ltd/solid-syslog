/** @file
 *  TLS over an injected byte-transport Stream (OpenSSL reference integration),
 *  for a StreamSender that needs an encrypted channel. The transport (a plain
 *  TcpStream, typically) carries the ciphertext; this stream owns the TLS.
 *
 *  What the stream does through its vtable is the substance:
 *
 *  - Open opens the transport, then builds a fresh SSL_CTX every call (the
 *    cert-rotation contract — a reconnect re-reads the cert/key files), pins the
 *    TLS 1.2 floor, loads CaBundlePath as the trust anchors with SSL_VERIFY_PEER,
 *    wires the transport as a custom BIO, sets SNI + the expected peer identity
 *    from ServerName, and drives the handshake. Any step failing closes the whole
 *    stream so the sender reconnects on its next pass.
 *  - The handshake is a bounded, non-blocking retry: SSL_connect is polled, and
 *    each WANT_READ / WANT_WRITE sleeps briefly via the injected Sleep until the
 *    handshake completes, hits a hard error (rejected), or the deadline from
 *    GetHandshakeTimeoutMs expires (re-read each attempt, so a runtime-tunable
 *    value applies on the next reconnect).
 *  - Send is all-or-nothing over SSL_write: a short write or any error is taken
 *    as a dead connection, so the stream closes itself and the sender reconnects.
 *  - Read returns the bytes read, 0 for would-block (WANT_READ, connection kept),
 *    or closes on anything else — including a mid-stream WANT_WRITE (renegotiation)
 *    which fail-fast semantics treat as a transport failure; store-and-forward
 *    replays after the reopen. */
#ifndef SOLIDSYSLOGTLSSTREAM_H
#define SOLIDSYSLOGTLSSTREAM_H

#include "ExternC.h"
#include "SolidSyslogSleep.h"
#include "SolidSyslogTlsHandshakeTimeoutFunction.h"

struct SolidSyslogStream;

EXTERN_C_BEGIN

    /** Wires SolidSyslogTlsStream to its transport, trust anchors, and identity. */
    struct SolidSyslogTlsStreamConfig
    {
        struct SolidSyslogStream* Transport; /**< Underlying byte stream carrying the ciphertext; caller owns it. */
        SolidSyslogSleepFunction Sleep; /**< Drives the bounded handshake retry between WANT_READ/WANT_WRITE
                                         *  polls; required. */
        SolidSyslogTlsHandshakeTimeoutFunction GetHandshakeTimeoutMs; /**< Per-attempt handshake deadline in ms;
                                                                       *  NULL uses the
                                                                       *  SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS
                                                                       *  tunable. */
        void* HandshakeTimeoutContext; /**< Passed back to GetHandshakeTimeoutMs unchanged; NULL is fine. */
        const char* CaBundlePath; /**< PEM file of trust anchors the peer cert must chain to. */
        const char* ServerName; /**< SNI plus the expected peer identity. A non-empty name is verified against the
                                 *  cert (SAN/CN). NULL connects chain-only but emits a WARNING — the peer is
                                 *  unverified (MITM-class). "" is the deliberate opt-out (IP-pinning / private
                                 *  CA): chain-only, no diagnostic. */
        const char* CipherList; /**< TLS 1.2 cipher list; NULL uses the OpenSSL default. */
        const char* ClientCertChainPath; /**< PEM leaf cert (+ intermediates) for mTLS; NULL = no mTLS. Cert and
                                          *  key are all-or-nothing — supplying one without the other is a setup
                                          *  error. */
        const char* ClientKeyPath; /**< PEM private key matching ClientCertChainPath; NULL = no mTLS. */
    };

    /** Draw a TLS stream from the pool over the injected transport (see the file
     *  overview for the handshake and I/O behaviour). An exhausted pool (default
     *  size 1) falls back to the shared NullStream. */
    struct SolidSyslogStream* SolidSyslogTlsStream_Create(const struct SolidSyslogTlsStreamConfig* config);
    /** Release the pool slot; closes the TLS session and the underlying transport
     *  first if the stream is still Open. */
    void SolidSyslogTlsStream_Destroy(struct SolidSyslogStream * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGTLSSTREAM_H */
