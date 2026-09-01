/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The no-op Mbed TLS credentials Null object: installs nothing and releases
 *  nothing. */
#ifndef SOLIDSYSLOGNULLMBEDTLSCREDENTIALS_H
#define SOLIDSYSLOGNULLMBEDTLSCREDENTIALS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogMbedTlsCredentials;

    /** Installs nothing and releases nothing. */
    struct SolidSyslogMbedTlsCredentials* SolidSyslogNullMbedTlsCredentials_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNULLMBEDTLSCREDENTIALS_H */
