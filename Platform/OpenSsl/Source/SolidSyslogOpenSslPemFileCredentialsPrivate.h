/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGOPENSSLPEMFILECREDENTIALSPRIVATE_H
#define SOLIDSYSLOGOPENSSLPEMFILECREDENTIALSPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogOpenSslCredentialsDefinition.h"
#include "SolidSyslogOpenSslPemFileCredentials.h"
#include "SolidSyslogOpenSslPemFileCredentialsErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamOpenHealth.h"

struct SolidSyslogOpenSslPemFileCredentials
{
    struct SolidSyslogOpenSslCredentials Base;
    struct SolidSyslogOpenSslPemFileCredentialsConfig Config;
    /* A refusal that fails Install is reported once per run of the same
     * detail, as a stream's failed Open is: the stream asks on every attempt.
     * An Install that succeeds re-arms it. */
    struct SolidSyslogStreamOpenHealth InstallHealth;
};

void SolidSyslogOpenSslPemFileCredentials_Initialise(
    struct SolidSyslogOpenSslCredentials* base,
    const struct SolidSyslogOpenSslPemFileCredentialsConfig* config
);

static inline void OpenSslPemFileCredentials_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogTlsCredentialsErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogOpenSslPemFileCredentialsErrorSource, category, (int32_t) code);
}

static inline void OpenSslPemFileCredentials_ReportInstallFailure(
    struct SolidSyslogOpenSslPemFileCredentials* self,
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogTlsCredentialsErrors code
)
{
    SolidSyslogStreamOpenHealth_ReportFailure(
        &self->InstallHealth,
        &SolidSyslogOpenSslPemFileCredentialsErrorSource,
        severity,
        category,
        (int32_t) code
    );
}

#endif /* SOLIDSYSLOGOPENSSLPEMFILECREDENTIALSPRIVATE_H */
