/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The no-op wolfSSL credentials Null object: installs nothing and releases
 *  nothing. */
#ifndef SOLIDSYSLOGWOLFSSLNULLCREDENTIALS_H
#define SOLIDSYSLOGWOLFSSLNULLCREDENTIALS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogWolfSslCredentials;

    /** Installs nothing and releases nothing. */
    struct SolidSyslogWolfSslCredentials* SolidSyslogWolfSslNullCredentials_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWOLFSSLNULLCREDENTIALS_H */
