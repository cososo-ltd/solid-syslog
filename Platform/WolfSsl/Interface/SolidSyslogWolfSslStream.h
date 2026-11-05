/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  TLS over an injected byte-transport Stream via wolfSSL, itself a Stream - so
 *  a StreamSender speaks TLS to a remote collector without knowing the transport
 *  underneath, whether a TCP stream from a platform pack or one the caller
 *  supplies.
 *
 *  What the stream does through its vtable is the substance:
 *
 *  - Open first opens the underlying transport, builds a client context for
 *    this connection, asks the credentials source to install the material onto
 *    it, creates the session, applies the library-owned TLS policy (TLS 1.2
 *    floor, the BCP 195 section 4.5 key-size floors, a required peer
 *    certificate), installs the peer identity, then drives the handshake to
 *    completion. The non-blocking transport means each wolfSSL_connect may want
 *    more I/O; the injected Sleep bridges those polls until the handshake
 *    completes, hits a hard error (HANDSHAKE_REJECTED, or the check the peer
 *    failed), or the bounded budget expires (HANDSHAKE_TIMEOUT - re-read from
 *    GetHandshakeTimeoutMs each attempt, so a runtime-tunable value applies on
 *    the next reconnect). A failed Open closes the stream so the sender
 *    reconnects on its next pass.
 *  - Send is all-or-nothing: a short write or any TLS error is taken as an
 *    unrecoverable session and closes the stream, so the sender reconnects.
 *  - Read returns the bytes read, 0 for would-block, or closes the stream on any
 *    other TLS return (alert, transport error) - fail-fast, and store-and-forward
 *    replays after the reconnect.
 *
 *  Peer identity comes from the profile's ServerName, asked for at each Open,
 *  and from any fingerprints the credentials source pins. No key material
 *  reaches this stream: it asks its credentials source to
 *  install onto the context at Open and tells it at Close, so a deployment can
 *  keep material out of memory between connections. The context and the
 *  session are both built per connection and freed at Close, so nothing of one
 *  connection survives into the next. Coexistence contract: this adapter calls
 *  no process-global wolfSSL API (wolfSSL_Init, wolfSSL_Cleanup, debugging
 *  hooks); the integrator initialises the library once, as
 *  docs/platforms/wolfssl/setup.md says. */
#ifndef SOLIDSYSLOGWOLFSSLSTREAM_H
#define SOLIDSYSLOGWOLFSSLSTREAM_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogSleep.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogTlsHandshakeTimeoutFunction.h"

struct SolidSyslogStream;
struct SolidSyslogWolfSslCredentials;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** What one connection is made with, supplied by the integrator at each Open.
     *  The stream zeroes this before asking, so a field left alone is one the
     *  integrator has no policy on and the library's own default stands. Anything
     *  supplied must stay valid until the connection closes. */
    struct SolidSyslogWolfSslProfile
    {
        /** SNI + peer-identity check. A non-empty name is verified against the
         *  peer certificate (SAN, then Common Name where the certificate carries
         *  no dNSName) and sent as the server name indication. One that parses
         *  as an address literal is verified against an iPAddress SAN where
         *  wolfSSL was built with WOLFSSL_IP_ALT_NAME; without that flag the
         *  library matches it as a DNS name, which an iPAddress entry never
         *  satisfies, and the peer is refused as PEER_NAME_MISMATCHED. NULL
         *  declares no name: the peer is only chain-authenticated and a WARNING
         *  says so (MITM-class) - unless the credentials pin it, which declares
         *  its identity more strongly than a name, and nothing is reported. ""
         *  is the no-name-check opt-out (closed network / private CA): the peer
         *  must still satisfy whatever the credentials installed, but the
         *  endpoint identity is not checked; no diagnostic. */
        const char* ServerName;
        /** Ciphersuite policy, as wolfSSL takes it: names separated by colons,
         *  each in wolfSSL's own spelling (`ECDHE-RSA-AES128-GCM-SHA256`,
         *  `TLS13-AES128-GCM-SHA256`) or the IANA one. One list covers both TLS
         *  versions, so a policy for a collector that may negotiate either names
         *  a suite for each. NULL leaves every ciphersuite the build enables.
         *  wolfSSL keeps a name it knows and drops one it does not without
         *  saying so; a list in which it knows no name is rejected, and that
         *  rejection fails the connection at Open as CIPHER_POLICY_REJECTED.
         *  Groups and signature algorithms are not selectable through it. */
        const char* CipherList;
    };

    /** Called at each Open to fill @p profile. Runs on the servicing thread, so a
     *  value the integrator changes elsewhere is read here rather than stored, and
     *  moving the stream's Version is what makes the change take effect.
     *  @p context is ProfileContext, passed through unchanged. */
    typedef void (*SolidSyslogWolfSslProfileFunction)(struct SolidSyslogWolfSslProfile* profile, void* context);

    /** Wires SolidSyslogWolfSslStream to its transport, trust anchors, and identity.
     *  Wiring only: every value a connection is made with arrives through the
     *  Credentials or the Profile, so nothing here goes stale when a deployment
     *  changes. */
    struct SolidSyslogWolfSslStreamConfig
    {
        /** Underlying byte stream the TLS records ride on; required - a NULL is
         *  reported at SolidSyslogWolfSslStream_Create. Borrowed - this stream
         *  may Close it but never destroys it; the caller owns it and must keep
         *  it valid until SolidSyslogWolfSslStream_Destroy. */
        struct SolidSyslogStream* Transport;
        /** Where the trust anchors, any pinned peer fingerprints and the mutual-TLS
         *  client credential come from; required - a NULL is reported at
         *  SolidSyslogWolfSslStream_Create. Asked once per connection, so material
         *  is fetched only for a connection actually being made, and told when the
         *  connection ends. Borrowed - the caller owns it and must keep it valid
         *  until SolidSyslogWolfSslStream_Destroy. */
        struct SolidSyslogWolfSslCredentials* Credentials;
        SolidSyslogSleepFunction Sleep; /**< Bridges the WANT_READ/WANT_WRITE polls of the bounded handshake
                                             retry; required - a NULL is reported at
                                             SolidSyslogWolfSslStream_Create. */
        SolidSyslogTlsHandshakeTimeoutFunction GetHandshakeTimeoutMs; /**< Per-attempt handshake deadline in ms;
                                             NULL uses the SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS tunable. */
        void* HandshakeTimeoutContext; /**< Passed back to GetHandshakeTimeoutMs unchanged; NULL is fine. */
        /** The heap hint every allocation this stream asks wolfSSL for is made
         *  against - the context, the session and what they hold - passed to
         *  wolfSSL_CTX_new_ex as it stands. NULL uses the library's default
         *  allocator. Meaningful on a build with WOLFSSL_STATIC_MEMORY or a
         *  custom allocator that reads the hint; other builds ignore it. */
        void* Heap;
        /** Supplies the per-connection profile. NULL leaves every profile field at
         *  the library default, which for ServerName means an unverified peer. */
        SolidSyslogWolfSslProfileFunction Profile;
        void* ProfileContext; /**< Passed to Profile unchanged; NULL is fine. */
        /** Bumped by the integrator when anything above changes at runtime - the
         *  Credentials, or anything the Profile supplies. The sender polls it every
         *  Send and reconnects on the next pass when it moves, so a rotation applies
         *  without calling SolidSyslogSender_Disconnect. Polled from the servicing
         *  thread, so it must be cheap and pure. NULL means this configuration
         *  never changes. */
        SolidSyslogStreamVersionFunction Version;
        void* VersionContext; /**< Passed to Version unchanged; NULL is fine. */
    };

    /** Draw a TLS stream from the pool over the config's Transport (see the file
     *  overview for the handshake and I/O behaviour). A NULL config, a NULL
     *  Transport, a NULL Sleep or a NULL Credentials is reported and falls back
     *  to the shared NullStream, as does an exhausted pool. */
    struct SolidSyslogStream* SolidSyslogWolfSslStream_Create(const struct SolidSyslogWolfSslStreamConfig* config);
    /** Release the pool slot; closes the TLS session and the underlying transport
     *  if the stream is still open. */
    void SolidSyslogWolfSslStream_Destroy(struct SolidSyslogStream * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWOLFSSLSTREAM_H */
