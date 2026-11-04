/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWolfSslSettings.h"

#include "SolidSyslogWolfSslPemBufferCredentials.h"

#include <stdbool.h>
#include <stddef.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogTlsCredentialsInstalled.h"
#include "SolidSyslogWolfSslCredentialsDefinition.h"
#include "SolidSyslogWolfSslPemBufferCredentialsPrivate.h"

const struct SolidSyslogErrorSource SolidSyslogWolfSslPemBufferCredentialsErrorSource = {
    "WolfSslPemBufferCredentials",
    SolidSyslogTlsCredentialsErrors_Name
};

static bool WolfSslPemBufferCredentials_Install(
    struct SolidSyslogWolfSslCredentials* base,
    struct WOLFSSL_CTX* ctx,
    struct SolidSyslogTlsCredentialsInstalled* installed
);
static inline bool WolfSslPemBufferCredentials_LoadTrustAnchors(
    const struct SolidSyslogWolfSslPemBufferCredentials* self,
    WOLFSSL_CTX* ctx
);
static inline void WolfSslPemBufferCredentials_ConfigureClientIdentity(
    const struct SolidSyslogWolfSslPemBufferCredentials* self,
    WOLFSSL_CTX* ctx
);
static inline bool WolfSslPemBufferCredentials_HasClientCredential(
    const struct SolidSyslogWolfSslPemBufferCredentialsConfig* config
);
static inline bool WolfSslPemBufferCredentials_HasHalfOfClientCredential(
    const struct SolidSyslogWolfSslPemBufferCredentialsConfig* config
);
static inline bool WolfSslPemBufferCredentials_ClientPemHasExtent(
    const struct SolidSyslogWolfSslPemBufferCredentialsConfig* config
);
static inline int WolfSslPemBufferCredentials_LoadClientCredential(
    const struct SolidSyslogWolfSslPemBufferCredentials* self,
    WOLFSSL_CTX* ctx
);
static inline bool WolfSslPemBufferCredentials_IsSupplied(const struct SolidSyslogWolfSslPemBuffer* pem);
static inline bool WolfSslPemBufferCredentials_HasExtent(const struct SolidSyslogWolfSslPemBuffer* pem);
static void WolfSslPemBufferCredentials_Release(struct SolidSyslogWolfSslCredentials* base);
static inline struct SolidSyslogWolfSslPemBufferCredentials* WolfSslPemBufferCredentials_SelfFromBase(
    struct SolidSyslogWolfSslCredentials* base
);

void SolidSyslogWolfSslPemBufferCredentials_Initialise(
    struct SolidSyslogWolfSslCredentials* base,
    const struct SolidSyslogWolfSslPemBufferCredentialsConfig* config
)
{
    struct SolidSyslogWolfSslPemBufferCredentials* self = WolfSslPemBufferCredentials_SelfFromBase(base);
    self->Base.Install = WolfSslPemBufferCredentials_Install;
    self->Base.Release = WolfSslPemBufferCredentials_Release;
    self->Config = *config;
}

static inline struct SolidSyslogWolfSslPemBufferCredentials* WolfSslPemBufferCredentials_SelfFromBase(
    struct SolidSyslogWolfSslCredentials* base
)
{
    return (struct SolidSyslogWolfSslPemBufferCredentials*) base;
}

void SolidSyslogWolfSslPemBufferCredentials_Cleanup(struct SolidSyslogWolfSslCredentials* base)
{
    (void) base;
}

/* Loaded here rather than at Create, so the material exists only for a
 * connection actually being made - and only inside the context the stream
 * built for it, which the stream frees at Close. Nothing is held here. */
static bool WolfSslPemBufferCredentials_Install(
    struct SolidSyslogWolfSslCredentials* base,
    struct WOLFSSL_CTX* ctx,
    struct SolidSyslogTlsCredentialsInstalled* installed
)
{
    struct SolidSyslogWolfSslPemBufferCredentials* self = WolfSslPemBufferCredentials_SelfFromBase(base);
    installed->TrustAnchorsInstalled = false;
    installed->Fingerprints = self->Config.PeerFingerprints;
    installed->FingerprintCount = self->Config.PeerFingerprintCount;
    bool ok = true;
    if (WolfSslPemBufferCredentials_IsSupplied(&self->Config.CaPem))
    {
        installed->TrustAnchorsInstalled = WolfSslPemBufferCredentials_LoadTrustAnchors(self, ctx);
        ok = installed->TrustAnchorsInstalled;
    }
    else
    {
        /* No anchors configured is legitimate - a pin alone can authorise. */
    }
    if (ok)
    {
        WolfSslPemBufferCredentials_ConfigureClientIdentity(self, ctx);
    }
    return ok;
}

/* A buffer with no extent is a mistake, not a decision to go without, and is
 * named as what it is rather than handed to a parser that would say "no PEM
 * header" about an empty buffer. */
static inline bool WolfSslPemBufferCredentials_LoadTrustAnchors(
    const struct SolidSyslogWolfSslPemBufferCredentials* self,
    WOLFSSL_CTX* ctx
)
{
    bool loaded =
        WolfSslPemBufferCredentials_HasExtent(&self->Config.CaPem) &&
        (wolfSSL_CTX_load_verify_buffer(
             ctx,
             self->Config.CaPem.Bytes,
             (long) self->Config.CaPem.Length,
             WOLFSSL_FILETYPE_PEM
         ) == WOLFSSL_SUCCESS);
    if (!loaded)
    {
        WolfSslPemBufferCredentials_Report(
            SOLIDSYSLOG_SEVERITY_ERROR,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_TRUST_ANCHORS_NOT_PARSED
        );
    }
    return loaded;
}

/* No fault in our own credential stops delivery: the collector is the
 * enforcement point for it, and one that requires a client certificate refuses
 * the handshake anyway. wolfSSL parses and installs in one call, so a failure
 * is "not parsed" unless the library says it ran out of memory holding it. A
 * certificate that loaded ahead of a key that did not is freed with the
 * context at Close, like everything else on it. */
static inline void WolfSslPemBufferCredentials_ConfigureClientIdentity(
    const struct SolidSyslogWolfSslPemBufferCredentials* self,
    WOLFSSL_CTX* ctx
)
{
    if (WolfSslPemBufferCredentials_HasClientCredential(&self->Config))
    {
        int rc = WolfSslPemBufferCredentials_LoadClientCredential(self, ctx);
        if (rc == MEMORY_E)
        {
            WolfSslPemBufferCredentials_Report(
                SOLIDSYSLOG_SEVERITY_WARNING,
                SOLIDSYSLOG_CAT_BAD_CONFIG,
                SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_CLIENT_CREDENTIAL_NOT_INSTALLED
            );
        }
        else if (rc != WOLFSSL_SUCCESS)
        {
            WolfSslPemBufferCredentials_Report(
                SOLIDSYSLOG_SEVERITY_WARNING,
                SOLIDSYSLOG_CAT_BAD_CONFIG,
                SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_CLIENT_CREDENTIAL_NOT_PARSED
            );
        }
        else
        {
            /* Parsed and installed - the credential will be presented. */
        }
    }
    else if (WolfSslPemBufferCredentials_HasHalfOfClientCredential(&self->Config))
    {
        WolfSslPemBufferCredentials_Report(
            SOLIDSYSLOG_SEVERITY_WARNING,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_CLIENT_CREDENTIAL_INCOMPLETE
        );
    }
    else
    {
        /* Neither supplied - server-authenticated TLS is the deliberate case. */
    }
}

static inline bool WolfSslPemBufferCredentials_HasClientCredential(
    const struct SolidSyslogWolfSslPemBufferCredentialsConfig* config
)
{
    return WolfSslPemBufferCredentials_IsSupplied(&config->ClientCertPem) &&
           WolfSslPemBufferCredentials_IsSupplied(&config->ClientKeyPem);
}

/* One half without the other. The integrator asked for mutual TLS and will not
 * get it, so it is reported rather than read as a decision to go without. */
static inline bool WolfSslPemBufferCredentials_HasHalfOfClientCredential(
    const struct SolidSyslogWolfSslPemBufferCredentialsConfig* config
)
{
    return WolfSslPemBufferCredentials_IsSupplied(&config->ClientCertPem) !=
           WolfSslPemBufferCredentials_IsSupplied(&config->ClientKeyPem);
}

/* Both halves, because a certificate whose key is unusable is no more use than
 * neither. */
static inline bool WolfSslPemBufferCredentials_ClientPemHasExtent(
    const struct SolidSyslogWolfSslPemBufferCredentialsConfig* config
)
{
    return WolfSslPemBufferCredentials_HasExtent(&config->ClientCertPem) &&
           WolfSslPemBufferCredentials_HasExtent(&config->ClientKeyPem);
}

/* The first call that does not succeed answers for the pair; a buffer with no
 * extent answers as the parser would for an empty one. */
static inline int WolfSslPemBufferCredentials_LoadClientCredential(
    const struct SolidSyslogWolfSslPemBufferCredentials* self,
    WOLFSSL_CTX* ctx
)
{
    int rc = WOLFSSL_FAILURE;
    if (WolfSslPemBufferCredentials_ClientPemHasExtent(&self->Config))
    {
        rc = wolfSSL_CTX_use_certificate_chain_buffer(
            ctx,
            self->Config.ClientCertPem.Bytes,
            (long) self->Config.ClientCertPem.Length
        );
        if (rc == WOLFSSL_SUCCESS)
        {
            rc = wolfSSL_CTX_use_PrivateKey_buffer(
                ctx,
                self->Config.ClientKeyPem.Bytes,
                (long) self->Config.ClientKeyPem.Length,
                WOLFSSL_FILETYPE_PEM
            );
        }
    }
    return rc;
}

/* A NULL buffer is the documented way to say "not supplied". */
static inline bool WolfSslPemBufferCredentials_IsSupplied(const struct SolidSyslogWolfSslPemBuffer* pem)
{
    return pem->Bytes != NULL;
}

static inline bool WolfSslPemBufferCredentials_HasExtent(const struct SolidSyslogWolfSslPemBuffer* pem)
{
    return pem->Length > 0U;
}

/* Nothing was acquired here: what Install loaded belongs to the context, and
 * the stream frees that before it calls this. */
static void WolfSslPemBufferCredentials_Release(struct SolidSyslogWolfSslCredentials* base)
{
    (void) base;
}
