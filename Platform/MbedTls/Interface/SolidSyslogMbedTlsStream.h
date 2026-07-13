/** @file
 *  TLS over an injected byte-transport Stream via Mbed TLS, itself a Stream — so
 *  a StreamSender speaks TLS to a remote collector without knowing the transport
 *  underneath (PosixTcpStream, PlusTcpTcpStream, or any caller-supplied byte
 *  Stream).
 *
 *  What the stream does through its vtable is the substance:
 *
 *  - Open first opens the underlying transport, applies the library-owned TLS
 *    policy (client mode, TLS 1.2 floor, VERIFY_REQUIRED against the CaChain),
 *    installs the peer identity, then drives the handshake to completion. The
 *    non-blocking transport means each mbedtls_ssl_handshake may want more I/O;
 *    the injected Sleep bridges those polls until the handshake completes, hits
 *    a hard error (HANDSHAKE_REJECTED), or the bounded budget expires
 *    (HANDSHAKE_TIMEOUT — re-read from GetHandshakeTimeoutMs each attempt, so a
 *    runtime-tunable value applies on the next reconnect). A failed Open closes
 *    the stream so the sender reconnects on its next pass.
 *  - Send is all-or-nothing: a short write or any TLS error is taken as an
 *    unrecoverable session and closes the stream, so the sender reconnects.
 *  - Read returns the bytes read, 0 for would-block, or closes the stream on any
 *    other TLS return (alert, transport error) — fail-fast, and store-and-forward
 *    replays after the reconnect.
 *
 *  Peer identity is set by ServerName (see the config member). All key material
 *  is injected as caller-built, caller-owned mbedTLS handles — never file paths
 *  or PEM blobs. Coexistence contract: this adapter touches only per-instance
 *  ssl_config / ssl_context state and never calls process-global mbedTLS APIs
 *  (platform setup/teardown, psa_crypto_init, threading-alt, debug hooks), so it
 *  drops into an integrator process that already uses Mbed TLS elsewhere. See
 *  docs/integrating-mbedtls.md. */
#ifndef SOLIDSYSLOGMBEDTLSSTREAM_H
#define SOLIDSYSLOGMBEDTLSSTREAM_H

#include "ExternC.h"
#include "SolidSyslogSleep.h"
#include "SolidSyslogTlsHandshakeTimeoutFunction.h"

struct SolidSyslogStream;

/* Forward declarations keep the public header free of any mbedTLS include.
 * Integrators include the relevant mbedTLS headers themselves before this
 * one to bring the types into scope. See project_mbedtls_di_handles. */
struct mbedtls_ctr_drbg_context;
struct mbedtls_x509_crt;
struct mbedtls_pk_context;

EXTERN_C_BEGIN

    struct SolidSyslogMbedTlsStreamConfig
    {
        struct SolidSyslogStream* Transport; /**< Underlying byte stream the TLS records ride on; caller owns it. */
        SolidSyslogSleepFunction Sleep; /**< Bridges the WANT_READ/WANT_WRITE polls of the bounded handshake
                                             retry; required — there is no fallback. */
        SolidSyslogTlsHandshakeTimeoutFunction GetHandshakeTimeoutMs; /**< Per-attempt handshake deadline in ms;
                                             NULL uses the SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS tunable. */
        void* HandshakeTimeoutContext; /**< Passed back to GetHandshakeTimeoutMs unchanged; NULL is fine. */
        struct mbedtls_ctr_drbg_context* Rng; /**< Seeded CTR-DRBG for the handshake; caller-built and caller-owned. */
        struct mbedtls_x509_crt* CaChain; /**< Trust anchors the peer cert must chain to; caller-built and owned. */
        const char* ServerName; /**< SNI + peer-identity check. A non-empty name is verified against the peer
                                     cert (SAN/CN). NULL connects chain-only but emits a WARNING — the peer is
                                     unverified (MITM-class). "" is the deliberate opt-out (IP-pinning / private
                                     CA): connect chain-only, no diagnostic. */
        struct mbedtls_x509_crt* ClientCertChain; /**< mTLS leaf (+ intermediates); caller-owned. NULL (or a NULL
                                     ClientKey) disables mTLS — both must be set to present a client cert. */
        struct mbedtls_pk_context* ClientKey; /**< Private key matching ClientCertChain; caller-owned. NULL disables
                                     mTLS. */
    };

    /** Draw a TLS stream from the pool over the config's Transport (see the file
     *  overview for the handshake and I/O behaviour). An exhausted pool falls back
     *  to the shared NullStream. */
    struct SolidSyslogStream* SolidSyslogMbedTlsStream_Create(const struct SolidSyslogMbedTlsStreamConfig* config);
    /** Release the pool slot; closes the TLS session and the underlying transport
     *  if the stream is still open. */
    void SolidSyslogMbedTlsStream_Destroy(struct SolidSyslogStream * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGMBEDTLSSTREAM_H */
