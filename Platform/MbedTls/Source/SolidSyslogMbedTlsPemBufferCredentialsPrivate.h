/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGMBEDTLSPEMBUFFERCREDENTIALSPRIVATE_H
#define SOLIDSYSLOGMBEDTLSPEMBUFFERCREDENTIALSPRIVATE_H

#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogMbedTlsCredentialsDefinition.h"
#include "SolidSyslogMbedTlsPemBufferCredentials.h"
#include "SolidSyslogMbedTlsPemBufferCredentialsErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamOpenHealth.h"

struct SolidSyslogMbedTlsPemBufferCredentials
{
    struct SolidSyslogMbedTlsCredentials Base;
    struct SolidSyslogMbedTlsPemBufferCredentialsConfig Config;
    /* Holds the parsed material for the length of one connection only. Init'd
     * at Create so the frees in Release are safe whether or not Install ever
     * ran, and left in Mbed TLS's freed-equivalent state afterwards so the next
     * Install parses into them again. */
    mbedtls_x509_crt CaChain;
    mbedtls_x509_crt ClientCertChain;
    mbedtls_pk_context ClientKey;
    /* Install/Release calls outstanding. One slot parses for one connection,
     * so a second overlapping Install is refused - but the contract answers
     * every Install with a Release whatever it returned, so counting is what
     * keeps a refused one from freeing the live connection's material. */
    size_t OutstandingInstalls;
    /* A refusal that fails Install is reported once per run of the same
     * detail, as a stream's failed Open is: the stream asks on every attempt.
     * An Install that succeeds re-arms it. */
    struct SolidSyslogStreamOpenHealth InstallHealth;
};

void SolidSyslogMbedTlsPemBufferCredentials_Initialise(
    struct SolidSyslogMbedTlsCredentials* base,
    const struct SolidSyslogMbedTlsPemBufferCredentialsConfig* config
);
void SolidSyslogMbedTlsPemBufferCredentials_Cleanup(struct SolidSyslogMbedTlsCredentials* base);

static inline void MbedTlsPemBufferCredentials_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogTlsCredentialsErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogMbedTlsPemBufferCredentialsErrorSource, category, (int32_t) code);
}

static inline void MbedTlsPemBufferCredentials_ReportInstallFailure(
    struct SolidSyslogMbedTlsPemBufferCredentials* self,
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogTlsCredentialsErrors code
)
{
    SolidSyslogStreamOpenHealth_ReportFailure(
        &self->InstallHealth,
        &SolidSyslogMbedTlsPemBufferCredentialsErrorSource,
        severity,
        category,
        (int32_t) code
    );
}

#endif /* SOLIDSYSLOGMBEDTLSPEMBUFFERCREDENTIALSPRIVATE_H */
