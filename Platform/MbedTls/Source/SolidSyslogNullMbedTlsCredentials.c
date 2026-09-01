/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogNullMbedTlsCredentials.h"

#include <stddef.h>

#include "SolidSyslogMbedTlsCredentialsDefinition.h"

struct SolidSyslogMbedTlsCredentials* SolidSyslogNullMbedTlsCredentials_Get(void)
{
    static struct SolidSyslogMbedTlsCredentials instance = {NULL, NULL};
    return &instance;
}
