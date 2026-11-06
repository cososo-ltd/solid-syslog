/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGWOLFSSLPEMBUFFERCREDENTIALSPRIVATE_H
#define SOLIDSYSLOGWOLFSSLPEMBUFFERCREDENTIALSPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamOpenHealth.h"
#include "SolidSyslogWolfSslCredentialsDefinition.h"
#include "SolidSyslogWolfSslPemBufferCredentials.h"
#include "SolidSyslogWolfSslPemBufferCredentialsErrors.h"

struct SolidSyslogWolfSslPemBufferCredentials
{
    struct SolidSyslogWolfSslCredentials Base;
    struct SolidSyslogWolfSslPemBufferCredentialsConfig Config;
    /* A refusal that fails Install is reported once per run of the same
     * detail, as a stream's failed Open is: the stream asks on every attempt.
     * An Install that succeeds re-arms it. */
    struct SolidSyslogStreamOpenHealth InstallHealth;
};

void SolidSyslogWolfSslPemBufferCredentials_Initialise(
    struct SolidSyslogWolfSslCredentials* base,
    const struct SolidSyslogWolfSslPemBufferCredentialsConfig* config
);
void SolidSyslogWolfSslPemBufferCredentials_Cleanup(struct SolidSyslogWolfSslCredentials* base);

static inline void WolfSslPemBufferCredentials_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogTlsCredentialsErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogWolfSslPemBufferCredentialsErrorSource, category, (int32_t) code);
}

static inline void WolfSslPemBufferCredentials_ReportInstallFailure(
    struct SolidSyslogWolfSslPemBufferCredentials* self,
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogTlsCredentialsErrors code
)
{
    SolidSyslogStreamOpenHealth_ReportFailure(
        &self->InstallHealth,
        &SolidSyslogWolfSslPemBufferCredentialsErrorSource,
        severity,
        category,
        (int32_t) code
    );
}

#endif /* SOLIDSYSLOGWOLFSSLPEMBUFFERCREDENTIALSPRIVATE_H */
