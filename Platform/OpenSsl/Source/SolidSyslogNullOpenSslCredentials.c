/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogNullOpenSslCredentials.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogOpenSslCredentialsDefinition.h"
#include "SolidSyslogTlsCredentialsInstalled.h"

static bool NullOpenSslCredentials_NullInstall(
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

static void NullOpenSslCredentials_NullRelease(struct SolidSyslogOpenSslCredentials* self)
{
    (void) self;
}

struct SolidSyslogOpenSslCredentials* SolidSyslogNullOpenSslCredentials_Get(void)
{
    static struct SolidSyslogOpenSslCredentials instance = {
        NullOpenSslCredentials_NullInstall,
        NullOpenSslCredentials_NullRelease
    };
    return &instance;
}
