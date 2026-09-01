/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGOPENSSLPEMFILECREDENTIALSPRIVATE_H
#define SOLIDSYSLOGOPENSSLPEMFILECREDENTIALSPRIVATE_H

#include "SolidSyslogOpenSslCredentialsDefinition.h"
#include "SolidSyslogOpenSslPemFileCredentials.h"

struct SolidSyslogOpenSslPemFileCredentials
{
    struct SolidSyslogOpenSslCredentials Base;
    struct SolidSyslogOpenSslPemFileCredentialsConfig Config;
};

void OpenSslPemFileCredentials_Initialise(
    struct SolidSyslogOpenSslCredentials* base,
    const struct SolidSyslogOpenSslPemFileCredentialsConfig* config
);

#endif /* SOLIDSYSLOGOPENSSLPEMFILECREDENTIALSPRIVATE_H */
