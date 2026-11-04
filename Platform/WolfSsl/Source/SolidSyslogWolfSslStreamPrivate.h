/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGWOLFSSLSTREAMPRIVATE_H
#define SOLIDSYSLOGWOLFSSLSTREAMPRIVATE_H

#include "SolidSyslogWolfSslSettings.h"

#include <stdbool.h>
#include <stdint.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/asn.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogStreamOpenHealth.h"
#include "SolidSyslogTlsCredentialsInstalled.h"
#include "SolidSyslogTlsStreamCategories.h"
#include "SolidSyslogWolfSslStream.h"
#include "SolidSyslogWolfSslStreamErrors.h"

struct SolidSyslogWolfSslStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogWolfSslStreamConfig Config;
    struct SolidSyslogStreamOpenHealth OpenHealth;
    /* Both built per connection at Open and freed at Close; NULL in between. */
    WOLFSSL_CTX* Context;
    WOLFSSL* Session;
    /* Set immediately before Install is called, cleared by the Release that
     * answers it. The contract is one Release per Install call whatever that
     * call returned, and Close is idempotent, so the flag is what keeps both
     * true at once. */
    bool CredentialsInstalled;
    /* What the last Install reported; valid only while CredentialsInstalled. */
    struct SolidSyslogTlsCredentialsInstalled Installed;
    /* Pulled at the start of each Open and read for the rest of it. */
    struct SolidSyslogWolfSslProfile Profile;
    /* The checks the verify callback saw the peer fail, as a set: wolfSSL
     * reports one error per call and may call more than once, and the refusal
     * names the check the contract puts first, whichever the library raised
     * last. */
    uint32_t RefusedChecks;
    /* Whether the leaf matched a pin, as the verify callback last found it. */
    bool PeerPinned;
    /* Where the verify callback parses a certificate the library found no
     * signer for. Held here rather than on the stack of a callback already
     * deep inside the handshake; valid only within that one call. */
    DecodedCert DecodedCertificate;
};

void SolidSyslogWolfSslStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogWolfSslStreamConfig* config
);
void SolidSyslogWolfSslStream_Cleanup(struct SolidSyslogStream* base);

static inline void WolfSslStream_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogTlsStreamErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogWolfSslStreamErrorSource, category, (int32_t) code);
}

/* A failed handshake, reported once per run of the same detail as the Open
 * contract requires: the sender retries every service pass. */
static inline void WolfSslStream_ReportHandshakeFailure(
    struct SolidSyslogWolfSslStream* self,
    enum SolidSyslogSeverity severity,
    enum SolidSyslogTlsStreamErrors code
)
{
    SolidSyslogStreamOpenHealth_ReportFailure(
        &self->OpenHealth,
        &SolidSyslogWolfSslStreamErrorSource,
        severity,
        SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED,
        (int32_t) code
    );
}

#endif /* SOLIDSYSLOGWOLFSSLSTREAMPRIVATE_H */
