/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGMBEDTLSSTREAMPRIVATE_H
#define SOLIDSYSLOGMBEDTLSSTREAMPRIVATE_H

#include <stdbool.h>
#include <stdint.h>

#include <mbedtls/ssl.h>

#include "SolidSyslogError.h"
#include "SolidSyslogMbedTlsStream.h"
#include "SolidSyslogMbedTlsStreamErrors.h"
#include "SolidSyslogTlsCredentialsInstalled.h"
#include "SolidSyslogTlsFingerprint.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamDefinition.h"

struct SolidSyslogMbedTlsStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogMbedTlsStreamConfig Config;
    mbedtls_ssl_config SslConfig;
    mbedtls_ssl_context SslContext;
    /* Set immediately before Install is called, cleared by the Release that
     * answers it. The contract is one Release per Install call whatever that
     * call returned, and Close is idempotent, so the flag is what keeps both
     * true at once. */
    bool CredentialsInstalled;
    /* What the last Install reported. Read after Install returns, by the verify
     * callback and by the hostname step; valid only while CredentialsInstalled. */
    struct SolidSyslogTlsCredentialsInstalled
        Installed; /* Pulled at the start of each Open and read for the rest of it. */
    struct SolidSyslogMbedTlsProfile Profile;
    /* What the verify callback itself refused on, and zero where it did not.
     * Verifying optionally makes our callback the enforcement point, and a
     * callback that refuses leaves mbedtls_ssl_get_verify_result answering
     * 0xFFFFFFFF - so the reason has to be recorded as the decision is taken
     * rather than deduced afterwards from a verdict that no longer carries it. */
    uint32_t RefusedVerdict;
    /* What the pin walk decided at the leaf, recorded there. A pin naming a hash
     * this build compiled out is the integrator's fault, not the peer's, and
     * nothing in the library's verdict distinguishes the two. */
    enum SolidSyslogTlsAuthorisation PinVerdict;
};

void SolidSyslogMbedTlsStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogMbedTlsStreamConfig* config
);
void SolidSyslogMbedTlsStream_Cleanup(struct SolidSyslogStream* base);

static inline void MbedTlsStream_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogTlsStreamErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogMbedTlsStreamErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGMBEDTLSSTREAMPRIVATE_H */
