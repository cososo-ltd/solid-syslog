/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGMBEDTLSSTREAMPRIVATE_H
#define SOLIDSYSLOGMBEDTLSSTREAMPRIVATE_H

#include <stdint.h>

#include <mbedtls/ssl.h>

#include "SolidSyslogError.h"
#include "SolidSyslogMbedTlsStream.h"
#include "SolidSyslogMbedTlsStreamErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamDefinition.h"

struct SolidSyslogMbedTlsStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogMbedTlsStreamConfig Config;
    mbedtls_ssl_config SslConfig;
    mbedtls_ssl_context SslContext;
};

void MbedTlsStream_Initialise(struct SolidSyslogStream* base, const struct SolidSyslogMbedTlsStreamConfig* config);
void MbedTlsStream_Cleanup(struct SolidSyslogStream* base);

static inline void MbedTlsStream_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogMbedTlsStreamErrors code
)
{
    SolidSyslog_Error(severity, &MbedTlsStreamErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGMBEDTLSSTREAMPRIVATE_H */
