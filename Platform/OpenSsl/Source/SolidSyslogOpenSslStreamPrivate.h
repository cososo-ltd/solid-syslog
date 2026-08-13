/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGOPENSSLSTREAMPRIVATE_H
#define SOLIDSYSLOGOPENSSLSTREAMPRIVATE_H

#include <stdint.h>

#include <openssl/bio.h>
#include <openssl/types.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogOpenSslStream.h"
#include "SolidSyslogOpenSslStreamErrors.h"

struct SolidSyslogOpenSslStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogOpenSslStreamConfig Config;
    SSL_CTX* Ctx;
    SSL* Ssl;
    BIO_METHOD* BioMethod;
};

void OpenSslStream_Initialise(struct SolidSyslogStream* base, const struct SolidSyslogOpenSslStreamConfig* config);
void OpenSslStream_Cleanup(struct SolidSyslogStream* base);

static inline void OpenSslStream_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogOpenSslStreamErrors code
)
{
    SolidSyslog_Error(severity, &OpenSslStreamErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGOPENSSLSTREAMPRIVATE_H */
