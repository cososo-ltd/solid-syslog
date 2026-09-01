/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogOpenSslPemFileCredentials.h"

#include <stddef.h>

#include "SolidSyslogOpenSslCredentialsDefinition.h"
#include "SolidSyslogOpenSslPemFileCredentialsPrivate.h"

void OpenSslPemFileCredentials_Initialise(
    struct SolidSyslogOpenSslCredentials* base,
    const struct SolidSyslogOpenSslPemFileCredentialsConfig* config
)
{
    struct SolidSyslogOpenSslPemFileCredentials* self = (struct SolidSyslogOpenSslPemFileCredentials*) base;
    self->Base.Install = NULL;
    self->Base.Release = NULL;
    self->Config = *config;
}
