/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The wolfSSL credentials backend that loads PEM from caller-owned buffers:
 *  trust anchors, an optional client credential for mutual TLS, and the pinned
 *  peer fingerprints the stream reads. Nothing is parsed at Create - each
 *  buffer is handed to wolfSSL at the stream's Open, onto the context built for
 *  that connection, and freed with it at Close. So the backend holds no
 *  material of its own between connections, a change to a buffer's contents
 *  applies on the next connection once the stream's Version moves, and one
 *  instance can serve several streams at once. */
#ifndef SOLIDSYSLOGWOLFSSLPEMBUFFERCREDENTIALS_H
#define SOLIDSYSLOGWOLFSSLPEMBUFFERCREDENTIALS_H

#include <stddef.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogWolfSslCredentials;

    /** A PEM buffer: one or more PEM blocks, as the file would hold them. The
     *  caller owns the storage, and it must outlive the connections made with
     *  it - it is read at each Open, not copied. */
    struct SolidSyslogWolfSslPemBuffer
    {
        /** The PEM text. NULL means not supplied. */
        const unsigned char* Bytes;
        /** How many bytes of @p Bytes are the PEM. wolfSSL reads exactly this
         *  extent, so no terminator is required; one may follow, and is not
         *  counted. A supplied buffer with a length of zero holds nothing to
         *  parse, and is reported as such rather than read as not supplied. */
        size_t Length;
    };

    /** Where this backend's material lives. */
    struct SolidSyslogWolfSslPemBufferCredentialsConfig
    {
        /** Trust anchors the peer certificate must chain to; an unsupplied
         *  buffer installs none, which leaves the peer authorised only if the
         *  stream has another means to do it. */
        struct SolidSyslogWolfSslPemBuffer CaPem;
        /** Leaf certificate (plus intermediates, leaf first) for mutual TLS.
         *  Certificate and key are all-or-nothing - supplying one without the
         *  other is reported. */
        struct SolidSyslogWolfSslPemBuffer ClientCertPem;
        /** Private key matching ClientCertPem. Must not be encrypted: no
         *  password can be supplied. Whether it matches the certificate is not
         *  checked here - a mismatch surfaces when the collector refuses the
         *  handshake. */
        struct SolidSyslogWolfSslPemBuffer ClientKeyPem;
        /** Fingerprints of certificates the peer may present, any one of which
         *  authorises it. Each is the RFC 5425 section 4.2.2 form: the IANA hash
         *  name, a colon, then the digest of the DER certificate as
         *  colon-separated hexadecimal bytes in either case, e.g.
         *  `sha-256:E1:2D:...`. NULL with a count of zero pins no peer; a count
         *  with no list behind it, or a NULL pin in one, is reported at Create,
         *  which returns the Null credentials. The array and the strings must
         *  outlive the credentials. */
        const char* const * PeerFingerprints;
        size_t PeerFingerprintCount;
    };

    /** Draw a credentials instance from the pool. A NULL config is reported
     *  and falls back to the shared Null credentials, as does an exhausted
     *  pool. */
    struct SolidSyslogWolfSslCredentials* SolidSyslogWolfSslPemBufferCredentials_Create(
        const struct SolidSyslogWolfSslPemBufferCredentialsConfig* config
    );
    /** Release the pool slot. */
    void SolidSyslogWolfSslPemBufferCredentials_Destroy(struct SolidSyslogWolfSslCredentials * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWOLFSSLPEMBUFFERCREDENTIALS_H */
