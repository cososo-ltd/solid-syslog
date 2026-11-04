/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The wolfSSL credentials vtable (Install / Release) - where a TLS stream's
 *  trust anchors, client credential and pinned peer fingerprints come from,
 *  and the extension point an implementor fills in.
 *
 *  The role is per-pack rather than backend-neutral because the material is
 *  irreducibly backend-typed: what Install configures is a WOLFSSL_CTX. A
 *  backend fetches material when a connection is being made and lets go of it
 *  when the connection ends, so a deployment can keep credentials out of memory
 *  in between - a PEM buffer parsed on demand, a secure element or a key store
 *  are all backends of this one role. */
#ifndef SOLIDSYSLOGWOLFSSLCREDENTIALSDEFINITION_H
#define SOLIDSYSLOGWOLFSSLCREDENTIALSDEFINITION_H

#include <stdbool.h>

#include "SolidSyslogExternC.h"

/* Forward declaration keeps the header free of any wolfSSL include, as the
 * stream header does. Integrators include wolfssl/ssl.h themselves before
 * this one to bring the type into scope. */
struct WOLFSSL_CTX;

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogTlsCredentialsInstalled;

    /** Where a WolfSslStream's credentials come from. Called once per
     *  connection each, so material is obtained only for a connection actually
     *  being made and released when it ends. */
    struct SolidSyslogWolfSslCredentials
    {
        /** Install trust anchors, any pinned peer fingerprints, and - for
         *  mutual TLS - the client credential onto @p ctx, reporting through
         *  @p installed what the stream cannot read back out of @p ctx. Called
         *  from Open, after the transport connects and before the session is
         *  created from the context, so what is installed here is what the
         *  session inherits.
         *
         *  @retval false the material that authorises the peer could not be
         *          produced; the connection attempt fails and the sender
         *          retries on its next pass. A fault in the client's own
         *          credential is reported by the backend and returns true -
         *          it never stops delivery. */
        bool (*Install)(
            struct SolidSyslogWolfSslCredentials* base,
            struct WOLFSSL_CTX* ctx,
            struct SolidSyslogTlsCredentialsInstalled* installed
        );
        /** Release what Install acquired. Called from Close exactly once for
         *  every Install call, whatever that call returned, so a backend needs
         *  no rollback of its own and the integrator is always told when the
         *  credential window has closed. Must be a safe no-op when nothing was
         *  acquired. */
        void (*Release)(struct SolidSyslogWolfSslCredentials* base);
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWOLFSSLCREDENTIALSDEFINITION_H */
