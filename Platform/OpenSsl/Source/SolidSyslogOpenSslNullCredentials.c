/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogOpenSslNullCredentials.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogOpenSslCredentialsDefinition.h"
#include "SolidSyslogTlsCredentialsInstalled.h"

static bool OpenSslNullCredentials_Install(
    struct SolidSyslogOpenSslCredentials* self,
    SSL_CTX* ctx,
    struct SolidSyslogTlsCredentialsInstalled* installed
)
{
    (void) self;
    (void) ctx;
    installed->TrustAnchorsInstalled = false;
    installed->Fingerprints = NULL;
    installed->FingerprintCount = 0;
    return true;
}

static void OpenSslNullCredentials_Release(struct SolidSyslogOpenSslCredentials* self)
{
    (void) self;
}

struct SolidSyslogOpenSslCredentials* SolidSyslogOpenSslNullCredentials_Get(void)
{
    static struct SolidSyslogOpenSslCredentials instance = {
        OpenSslNullCredentials_Install,
        OpenSslNullCredentials_Release
    };
    return &instance;
}
