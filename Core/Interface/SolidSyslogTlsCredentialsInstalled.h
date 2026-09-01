/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  What a credentials backend tells the TLS stream it installed - the facts the
 *  stream needs but cannot read back out of the TLS library's own context.
 *
 *  Backend-neutral by design: every TLS pack defines its own credentials role,
 *  because the material is backend-typed, but what the stream needs to know
 *  about that material is the same whichever library holds it. */
#ifndef SOLIDSYSLOGTLSCREDENTIALSINSTALLED_H
#define SOLIDSYSLOGTLSCREDENTIALSINSTALLED_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Filled in by a credentials backend's Install, read by the stream that
     *  called it. */
    struct SolidSyslogTlsCredentialsInstalled
    {
        /** Trust anchors were installed, so the peer's certificate can be
         *  validated against a chain. A backend sets this itself rather than
         *  leaving it - the stream reads it to decide whether the TLS library
         *  can be left as the enforcement point, so a backend that installs
         *  anchors and does not say so verifies nothing. */
        bool TrustAnchorsInstalled;
        /** Certificate fingerprints authorising the peer, in the RFC 5425
         *  §4.2.2 form. Any one of them authorises, so a collector's old and
         *  new certificate can both be pinned across a renewal. The strings
         *  are the backend's and must stay valid until its Release. NULL with
         *  a count of zero where no peer is pinned. */
        const char* const* Fingerprints;
        size_t FingerprintCount;
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGTLSCREDENTIALSINSTALLED_H */
