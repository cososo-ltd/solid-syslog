/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogOpenSslPemFileCredentials.h"

#include <stdbool.h>
#include <stddef.h>

#include <openssl/ssl.h>

#include "SolidSyslogOpenSslCredentialsDefinition.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogOpenSslPemFileCredentialsPrivate.h"
#include "SolidSyslogTlsCredentialsInstalled.h"

const struct SolidSyslogErrorSource OpenSslPemFileCredentialsErrorSource = {"OpenSslPemFileCredentials"};

static bool OpenSslPemFileCredentials_Install(
    struct SolidSyslogOpenSslCredentials* base,
    SSL_CTX* ctx,
    struct SolidSyslogTlsCredentialsInstalled* installed
);

void OpenSslPemFileCredentials_Initialise(
    struct SolidSyslogOpenSslCredentials* base,
    const struct SolidSyslogOpenSslPemFileCredentialsConfig* config
)
{
    struct SolidSyslogOpenSslPemFileCredentials* self = (struct SolidSyslogOpenSslPemFileCredentials*) base;
    self->Base.Install = OpenSslPemFileCredentials_Install;
    self->Base.Release = NULL;
    self->Config = *config;
}

/* Trust anchors are named by path and opened by OpenSSL - this library performs
 * no file handling of its own, so the PEM bytes never pass through it. The
 * paths are re-read here rather than at Create, so a credential replaced while
 * the device runs is picked up on the next connection. */
static bool OpenSslPemFileCredentials_Install(
    struct SolidSyslogOpenSslCredentials* base,
    SSL_CTX* ctx,
    struct SolidSyslogTlsCredentialsInstalled* installed
)
{
    struct SolidSyslogOpenSslPemFileCredentials* self = (struct SolidSyslogOpenSslPemFileCredentials*) base;
    bool ok = true;
    installed->TrustAnchorsInstalled = false;
    installed->Fingerprints = NULL;
    installed->FingerprintCount = 0U;
    if (self->Config.CaBundlePath != NULL)
    {
        installed->TrustAnchorsInstalled = SSL_CTX_load_verify_locations(ctx, self->Config.CaBundlePath, NULL) == 1;
        ok = installed->TrustAnchorsInstalled;
        if (!ok)
        {
            OpenSslPemFileCredentials_Report(
                SOLIDSYSLOG_SEVERITY_ERROR,
                SOLIDSYSLOG_CAT_BAD_CONFIG,
                SOLIDSYSLOG_OPENSSL_PEM_FILE_CREDENTIALS_ERROR_TRUST_ANCHORS_NOT_LOADED
            );
        }
    }
    return ok;
}
