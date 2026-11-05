/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWolfSslNullCredentials.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogTlsCredentialsInstalled.h"
#include "SolidSyslogWolfSslCredentialsDefinition.h"

static bool WolfSslNullCredentials_Install(
    struct SolidSyslogWolfSslCredentials* self,
    struct WOLFSSL_CTX* ctx,
    struct SolidSyslogTlsCredentialsInstalled* installed
)
{
    (void) self;
    (void) ctx;
    installed->TrustAnchorsInstalled = false;
    installed->Fingerprints = NULL;
    installed->FingerprintCount = 0U;
    return true;
}

static void WolfSslNullCredentials_Release(struct SolidSyslogWolfSslCredentials* self)
{
    (void) self;
}

struct SolidSyslogWolfSslCredentials* SolidSyslogWolfSslNullCredentials_Get(void)
{
    static struct SolidSyslogWolfSslCredentials instance = {
        WolfSslNullCredentials_Install,
        WolfSslNullCredentials_Release
    };
    return &instance;
}
