/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGOPENSSLSTREAMPRIVATE_H
#define SOLIDSYSLOGOPENSSLSTREAMPRIVATE_H

#include <stdbool.h>
#include <stdint.h>

#include <openssl/bio.h>
#include <openssl/types.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogOpenSslStream.h"
#include "SolidSyslogOpenSslStreamErrors.h"
#include "SolidSyslogTlsCredentialsInstalled.h"
#include "SolidSyslogTlsFingerprint.h"

struct SolidSyslogOpenSslStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogOpenSslStreamConfig Config;
    SSL_CTX* Ctx;
    SSL* Ssl;
    BIO_METHOD* BioMethod;
    /* Set immediately before Install is called, cleared by the Release that
     * answers it. The contract is one Release per Install call whatever that
     * call returned, and Close is idempotent, so the flag is what keeps both
     * true at once. */
    bool CredentialsInstalled;
    /* What the last Install reported. Read after Install returns, by the verify
     * callback and by the hostname step; valid only while CredentialsInstalled. */
    struct SolidSyslogTlsCredentialsInstalled Installed;
    /* Pulled at the start of each Open and read for the rest of it. */
    struct SolidSyslogOpenSslProfile Profile;
    /* A chain-trust objection carried past a deeper certificate so the leaf's
     * pin could still be compared, and X509_V_OK where there was none. OpenSSL
     * abandons verification on the first refusal, so refusing at depth would
     * leave the pin uncompared; whether the objection is forgiven is the
     * leaf's decision, and this is what carries it there. */
    int ChainObjection;
    /* What the pin walk decided at the leaf, recorded there. A pin naming a hash
     * this build cannot compute is the integrator's fault, not the peer's, and
     * nothing in the library's verdict distinguishes the two. */
    enum SolidSyslogTlsAuthorisation PinVerdict;
};

void SolidSyslogOpenSslStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogOpenSslStreamConfig* config
);
void SolidSyslogOpenSslStream_Cleanup(struct SolidSyslogStream* base);

static inline void OpenSslStream_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogTlsStreamErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogOpenSslStreamErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGOPENSSLSTREAMPRIVATE_H */
