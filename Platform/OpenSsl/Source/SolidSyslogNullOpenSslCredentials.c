/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogNullOpenSslCredentials.h"

#include <stddef.h>

#include "SolidSyslogOpenSslCredentialsDefinition.h"

struct SolidSyslogOpenSslCredentials* SolidSyslogNullOpenSslCredentials_Get(void)
{
    static struct SolidSyslogOpenSslCredentials instance = {NULL, NULL};
    return &instance;
}
