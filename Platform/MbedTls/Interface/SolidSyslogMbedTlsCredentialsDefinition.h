/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The Mbed TLS credentials vtable (Install / Release) - where a TLS stream's
 *  trust anchors, client credential and pinned peer fingerprints come from,
 *  and the extension point an implementor fills in.
 *
 *  The role is per-pack rather than backend-neutral because the material is
 *  irreducibly backend-typed: what Install configures is an mbedtls_ssl_config.
 *  A backend fetches material when a connection is being made and lets go of it
 *  when the connection ends, so a deployment can keep credentials out of memory
 *  in between - caller-built handles, a PEM buffer parsed on demand, a secure
 *  element or a PSA opaque key are all backends of this one role. */
#ifndef SOLIDSYSLOGMBEDTLSCREDENTIALSDEFINITION_H
#define SOLIDSYSLOGMBEDTLSCREDENTIALSDEFINITION_H

#include <stdbool.h>

#include "SolidSyslogExternC.h"

/* Forward declarations keep the header free of any mbedTLS include, as the
 * stream header does. Integrators include the relevant mbedTLS headers
 * themselves before this one to bring the types into scope. */
struct mbedtls_ssl_config;

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogTlsCredentialsInstalled;

    /** Where an MbedTlsStream's credentials come from. Called once per
     *  connection each, so material is obtained only for a connection actually
     *  being made and released when it ends. */
    struct SolidSyslogMbedTlsCredentials
    {
        /** Install trust anchors, any pinned peer fingerprints, and - for
         *  mutual TLS - the client credential onto @p conf, reporting through
         *  @p installed what the stream cannot read back out of @p conf.
         *  Called from Open, after the transport connects.
         *
         *  @retval false the material that authorises the peer could not be
         *          produced; the connection attempt fails and the sender
         *          retries on its next pass. A fault in the client's own
         *          credential is reported by the backend and returns true -
         *          it never stops delivery. */
        bool (*Install)(
            struct SolidSyslogMbedTlsCredentials* self,
            struct mbedtls_ssl_config* conf,
            struct SolidSyslogTlsCredentialsInstalled* installed
        );
        /** Release what Install acquired. Called from Close exactly once for
         *  every Install call, whatever that call returned, so a backend needs
         *  no rollback of its own and the integrator is always told when the
         *  credential window has closed. Must be a safe no-op when nothing was
         *  acquired. */
        void (*Release)(struct SolidSyslogMbedTlsCredentials* self);
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGMBEDTLSCREDENTIALSDEFINITION_H */
