/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  An Mbed TLS credentials backend that carries caller-built, caller-owned
 *  mbedTLS handles. The integrator parses its own PEM, unwraps its own key, or
 *  fetches material from wherever it lives, and hands the resulting handles
 *  here; this library parses nothing and owns nothing.
 *
 *  The handles must outlive the credentials, because every connection installs
 *  the same ones. A backend that acquires material per connection - one parsing
 *  a PEM buffer on demand, or reaching a secure element - is a different
 *  implementation of the same role. */
#ifndef SOLIDSYSLOGMBEDTLSHANDLECREDENTIALS_H
#define SOLIDSYSLOGMBEDTLSHANDLECREDENTIALS_H

#include "SolidSyslogExternC.h"

/* Forward declarations keep the header free of any mbedTLS include, as the
 * stream header does. Integrators include the relevant mbedTLS headers
 * themselves before this one to bring the types into scope. */
struct mbedtls_ctr_drbg_context;
struct mbedtls_pk_context;
struct mbedtls_x509_crt;

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogMbedTlsCredentials;

    /** Where this backend's material lives. Every handle is caller-built and
     *  caller-owned, and must stay valid for the lifetime of the credentials. */
    struct SolidSyslogMbedTlsHandleCredentialsConfig
    {
        /** Trust anchors the peer certificate must chain to; NULL installs
         *  none, which leaves the peer authorised only if the stream has
         *  another means to do it. */
        struct mbedtls_x509_crt* CaChain;
        /** Leaf certificate (plus intermediates) for mutual TLS; NULL means no
         *  client credential. Certificate and key are all-or-nothing -
         *  supplying one without the other is reported. */
        struct mbedtls_x509_crt* ClientCertChain;
        /** Private key matching ClientCertChain; NULL means no client
         *  credential. */
        struct mbedtls_pk_context* ClientKey;
        /** Seeded CTR-DRBG, used to check the client key against its
         *  certificate; required - a NULL is reported at
         *  SolidSyslogMbedTlsHandleCredentials_Create. The stream takes its own
         *  handshake RNG separately, and the same one serves both. */
        struct mbedtls_ctr_drbg_context* Rng;
    };

    /** Draw a credentials instance from the pool. A NULL config or a NULL Rng is
     *  reported and falls back to the shared Null credentials, as does an
     *  exhausted pool. */
    struct SolidSyslogMbedTlsCredentials* SolidSyslogMbedTlsHandleCredentials_Create(
        const struct SolidSyslogMbedTlsHandleCredentialsConfig* config
    );
    /** Release the pool slot. */
    void SolidSyslogMbedTlsHandleCredentials_Destroy(struct SolidSyslogMbedTlsCredentials * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGMBEDTLSHANDLECREDENTIALS_H */
