/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogNullMbedTlsCredentials.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogMbedTlsCredentialsDefinition.h"
#include "SolidSyslogTlsCredentialsInstalled.h"

static bool NullMbedTlsCredentials_NullInstall(
    struct SolidSyslogMbedTlsCredentials* self,
    struct mbedtls_ssl_config* conf,
    struct SolidSyslogTlsCredentialsInstalled* installed
)
{
    (void) self;
    (void) conf;
    (void) installed;
    return true;
}

struct SolidSyslogMbedTlsCredentials* SolidSyslogNullMbedTlsCredentials_Get(void)
{
    static struct SolidSyslogMbedTlsCredentials instance = {NullMbedTlsCredentials_NullInstall, NULL};
    return &instance;
}
