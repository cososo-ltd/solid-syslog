/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWolfSslSettings.h"

#include "SolidSyslogWolfSslStream.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <wolfssl/ssl.h>
#include <wolfssl/error-ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/hash.h>

/* wolfSSL compiles its notBefore / notAfter checks out entirely under
 * NO_ASN_TIME, so no certificate in the chain is date-checked and nothing says
 * so at runtime. The TLS contract requires the validity period to be checked,
 * which is a promise that would then silently not hold, so the build stops
 * here rather than shipping it.
 *
 * A target with no clock cannot check dates and is a legitimate deployment.
 * Define SOLIDSYSLOG_WOLFSSL_NO_VALIDITY_CHECK to say so deliberately; the
 * obligation is then knowingly unmet rather than quietly missing. */
#if defined(NO_ASN_TIME) && !defined(SOLIDSYSLOG_WOLFSSL_NO_VALIDITY_CHECK)
#error \
    "NO_ASN_TIME is set, so certificate validity is not checked. Build wolfSSL with ASN time, or define SOLIDSYSLOG_WOLFSSL_NO_VALIDITY_CHECK to accept that."
#endif

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTlsCredentialsInstalled.h"
#include "SolidSyslogTlsFingerprint.h"
#include "SolidSyslogTlsStreamCategories.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWolfSslCredentialsDefinition.h"
#include "SolidSyslogWolfSslStreamErrors.h"
#include "SolidSyslogWolfSslStreamPrivate.h"

const struct SolidSyslogErrorSource SolidSyslogWolfSslStreamErrorSource = {
    "WolfSslStream",
    SolidSyslogTlsStreamErrors_Name
};

/* The handshake poll, the read-before-write probe, and the BCP 195 section 4.5
 * cryptographic floor in the units each wolfSSL setter takes. */
enum
{
    HANDSHAKE_POLL_INTERVAL_MILLISECONDS = 10U,
    WOLFSSL_STREAM_DISCARD_BYTES = 16U,
    WOLFSSL_STREAM_MAX_READS_BEFORE_WRITE = 8U,
    WOLFSSL_STREAM_MIN_RSA_KEY_BITS = 2048,
    WOLFSSL_STREAM_MIN_ECC_KEY_BITS = 224,
    WOLFSSL_STREAM_MIN_DH_KEY_BITS = 2048U
};

/* The checks the verify callback can see the peer fail, as bits of
 * RefusedChecks. */
enum
{
    WOLFSSL_STREAM_CHECK_CHAIN_TRUST = 1U,
    WOLFSSL_STREAM_CHECK_NAME = 2U,
    WOLFSSL_STREAM_CHECK_NOT_AFTER = 4U,
    WOLFSSL_STREAM_CHECK_NOT_BEFORE = 8U,
    WOLFSSL_STREAM_CHECK_FINGERPRINT = 16U
};

struct SolidSyslogAddress;

static uint32_t WolfSslStream_NullHandshakeTimeoutGetter(void* context);
static uint32_t WolfSslStream_NullVersion(void* context);
static void WolfSslStream_NullProfile(struct SolidSyslogWolfSslProfile* profile, void* context);
static inline bool WolfSslStream_ConfigProvidesHandshakeGetter(const struct SolidSyslogWolfSslStreamConfig* config);
static inline bool WolfSslStream_ConfigProvidesVersion(const struct SolidSyslogWolfSslStreamConfig* config);
static inline bool WolfSslStream_ConfigProvidesProfile(const struct SolidSyslogWolfSslStreamConfig* config);
static inline struct SolidSyslogWolfSslStream* WolfSslStream_SelfFromBase(struct SolidSyslogStream* base);
static inline bool WolfSslStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);
static inline bool WolfSslStream_IsOpen(const struct SolidSyslogWolfSslStream* self);
static inline bool WolfSslStream_Establish(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);
static inline void WolfSslStream_PullProfile(struct SolidSyslogWolfSslStream* self);
static inline bool WolfSslStream_BuildContext(struct SolidSyslogWolfSslStream* self);
static inline bool WolfSslStream_InstallCredentials(struct SolidSyslogWolfSslStream* self);
static inline bool WolfSslStream_PeerIsAuthorisable(const struct SolidSyslogTlsCredentialsInstalled* installed);
static inline bool WolfSslStream_FingerprintsAreUsable(const struct SolidSyslogTlsCredentialsInstalled* installed);
static inline bool WolfSslStream_CreateSession(struct SolidSyslogWolfSslStream* self);
static inline bool WolfSslStream_ApplyTlsPolicy(struct SolidSyslogWolfSslStream* self);
static inline bool WolfSslStream_ConfigureExpectedPeerName(struct SolidSyslogWolfSslStream* self);
static inline bool WolfSslStream_ApplyExpectedPeerName(struct SolidSyslogWolfSslStream* self, const char* serverName);
static inline int WolfSslStream_DeclareExpectedPeer(struct SolidSyslogWolfSslStream* self, const char* serverName);
static inline bool WolfSslStream_IsAddressLiteral(const char* name);
static inline bool WolfSslStream_IsDigitOrDot(char c);
static inline enum SolidSyslogTlsStreamErrors WolfSslStream_InitDetail(
    int rc,
    enum SolidSyslogTlsStreamErrors otherwise
);
static inline void WolfSslStream_InstallTransportCallbacks(struct SolidSyslogWolfSslStream* self);
static inline void WolfSslStream_ApplyPeerVerificationPolicy(struct SolidSyslogWolfSslStream* self);
static int WolfSslStream_VerifyPeer(int preverify, WOLFSSL_X509_STORE_CTX* store);
static inline bool WolfSslStream_LeafMatchesAPin(struct SolidSyslogWolfSslStream* self, WOLFSSL_X509_STORE_CTX* store);
static bool WolfSslStream_DigestCertificate(
    void* context,
    enum SolidSyslogTlsHashAlgorithm algorithm,
    uint8_t* digest,
    size_t* length
);
static inline enum wc_HashType WolfSslStream_HashTypeFor(enum SolidSyslogTlsHashAlgorithm algorithm);
static inline uint32_t WolfSslStream_CheckFailedFor(int error);
static inline uint32_t WolfSslStream_ResolveHandshakeTimeoutMs(struct SolidSyslogWolfSslStream* self);
static inline bool WolfSslStream_PerformHandshake(struct SolidSyslogWolfSslStream* self);
static inline bool WolfSslStream_IsRetryableHandshakeError(int error);
static inline bool WolfSslStream_IsHandshakeBudgetExhausted(uint32_t totalSleptMs, uint32_t budgetMs);
static inline uint32_t WolfSslStream_HandshakePollMs(uint32_t totalSleptMs, uint32_t budgetMs);
static inline enum SolidSyslogTlsStreamErrors WolfSslStream_RefusalDetail(const struct SolidSyslogWolfSslStream* self);
static inline bool WolfSslStream_Failed(const struct SolidSyslogWolfSslStream* self, uint32_t check);
static inline bool WolfSslStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size);
static bool WolfSslStream_SessionIsStillOpen(struct SolidSyslogStream* base);
static inline SolidSyslogSsize WolfSslStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size);
static inline void WolfSslStream_Close(struct SolidSyslogStream* base);
static inline void WolfSslStream_ReleaseCredentials(struct SolidSyslogWolfSslStream* self);
static uint32_t WolfSslStream_Version(struct SolidSyslogStream* base);
static int WolfSslStream_IoSend(WOLFSSL* ssl, char* buf, int sz, void* ctx);
static int WolfSslStream_IoRecv(WOLFSSL* ssl, char* buf, int sz, void* ctx);

void SolidSyslogWolfSslStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogWolfSslStreamConfig* config
)
{
    struct SolidSyslogWolfSslStream* self = WolfSslStream_SelfFromBase(base);
    self->Base.Open = WolfSslStream_Open;
    self->Base.Send = WolfSslStream_Send;
    self->Base.Read = WolfSslStream_Read;
    self->Base.Close = WolfSslStream_Close;
    self->Base.Version = WolfSslStream_Version;
    self->Config = *config;
    self->Context = NULL;
    self->Session = NULL;
    self->CredentialsInstalled = false;
    self->OpenHealth = (struct SolidSyslogStreamOpenHealth) {0};
    if (WolfSslStream_ConfigProvidesHandshakeGetter(config) == false)
    {
        /* Substitute the Null Object so the bounded-handshake loop has a
         * single code path regardless of whether the integrator wired
         * runtime tuning. */
        self->Config.GetHandshakeTimeoutMs = WolfSslStream_NullHandshakeTimeoutGetter;
        self->Config.HandshakeTimeoutContext = NULL;
    }
    if (WolfSslStream_ConfigProvidesVersion(config) == false)
    {
        self->Config.Version = WolfSslStream_NullVersion;
        self->Config.VersionContext = NULL;
    }
    if (WolfSslStream_ConfigProvidesProfile(config) == false)
    {
        self->Config.Profile = WolfSslStream_NullProfile;
        self->Config.ProfileContext = NULL;
    }
}

/* Null Object substituted in Initialise when the integrator does not install a
 * getter - returns the compile-time tunable so the bounded-handshake path is a
 * single code path regardless of whether the integrator wired runtime tuning. */
static uint32_t WolfSslStream_NullHandshakeTimeoutGetter(void* context)
{
    (void) context;
    return (uint32_t) SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS;
}

static inline bool WolfSslStream_ConfigProvidesHandshakeGetter(const struct SolidSyslogWolfSslStreamConfig* config)
{
    return (config != NULL) && (config->GetHandshakeTimeoutMs != NULL);
}

/* Null Object substituted at Initialise when the integrator installs no version
 * function - reports an unchanging configuration, so the sender never reconnects
 * on this stream's account. */
static uint32_t WolfSslStream_NullVersion(void* context)
{
    (void) context;
    return 0U;
}

static inline bool WolfSslStream_ConfigProvidesVersion(const struct SolidSyslogWolfSslStreamConfig* config)
{
    return (config != NULL) && (config->Version != NULL);
}

/* Null Object substituted at Initialise when the integrator supplies no profile -
 * leaves every field at the library default. */
static void WolfSslStream_NullProfile(struct SolidSyslogWolfSslProfile* profile, void* context)
{
    (void) context;
    (void) profile;
}

static inline bool WolfSslStream_ConfigProvidesProfile(const struct SolidSyslogWolfSslStreamConfig* config)
{
    return (config != NULL) && (config->Profile != NULL);
}

static inline struct SolidSyslogWolfSslStream* WolfSslStream_SelfFromBase(struct SolidSyslogStream* base)
{
    return (struct SolidSyslogWolfSslStream*) base;
}

void SolidSyslogWolfSslStream_Cleanup(struct SolidSyslogStream* base)
{
    /* An integrator who destroys a still-Open stream must not leak the
     * underlying TLS state. */
    WolfSslStream_Close(base);
    /* Overwrite the abstract base with the shared NullStream vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullStream_Get();
}

/* Idempotent: everything freed here is set NULL as it goes, so a second Close
 * finds nothing to free, and the transport's own Close is idempotent on every
 * Stream impl. close_notify is sent once and not waited for - the collector's
 * reply would need another service pass to arrive, and nothing is read on a
 * session being torn down. */
static inline void WolfSslStream_Close(struct SolidSyslogStream* base)
{
    struct SolidSyslogWolfSslStream* self = WolfSslStream_SelfFromBase(base);
    if (self->Session != NULL)
    {
        (void) wolfSSL_shutdown(self->Session);
        wolfSSL_free(self->Session);
        self->Session = NULL;
    }
    /* The context holds the credentials' material until it is freed, so the
       credentials are told the window has closed only after wolfSSL has let go
       of it. */
    if (self->Context != NULL)
    {
        wolfSSL_CTX_free(self->Context);
        self->Context = NULL;
    }
    WolfSslStream_ReleaseCredentials(self);
    SolidSyslogStream_Close(self->Config.Transport);
}

/* Answers every Install, so the integrator is always told when the credential
 * window has closed - including on the paths where Open failed part way. */
static inline void WolfSslStream_ReleaseCredentials(struct SolidSyslogWolfSslStream* self)
{
    if (self->CredentialsInstalled)
    {
        self->CredentialsInstalled = false;
        self->Config.Credentials->Release(self->Config.Credentials);
    }
}

static uint32_t WolfSslStream_Version(struct SolidSyslogStream* base)
{
    struct SolidSyslogWolfSslStream* self = WolfSslStream_SelfFromBase(base);
    return self->Config.Version(self->Config.VersionContext);
}

static inline bool WolfSslStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr)
{
    struct SolidSyslogWolfSslStream* self = WolfSslStream_SelfFromBase(base);
    bool ok = false;
    if (WolfSslStream_IsOpen(self))
    {
        /* Refused by contract, not reconnected: what is held stays as it is. */
        WolfSslStream_Report(
            SOLIDSYSLOG_BAD_ARGUMENT_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_ARGUMENT,
            SOLIDSYSLOG_TLS_STREAM_ERROR_ALREADY_OPEN
        );
    }
    else
    {
        ok = WolfSslStream_Establish(base, addr);
        if (ok)
        {
            SolidSyslogStreamOpenHealth_Opened(&self->OpenHealth);
        }
    }
    return ok;
}

/* Every Open that succeeds installs the credentials and every Close releases
 * them, so holding them is the one mark of a session still open. */
static inline bool WolfSslStream_IsOpen(const struct SolidSyslogWolfSslStream* self)
{
    return self->CredentialsInstalled;
}

static inline bool WolfSslStream_Establish(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr)
{
    struct SolidSyslogWolfSslStream* self = WolfSslStream_SelfFromBase(base);
    WolfSslStream_PullProfile(self);
    self->RefusedChecks = 0U;
    bool ok = SolidSyslogStream_Open(self->Config.Transport, addr) && WolfSslStream_BuildContext(self) &&
              WolfSslStream_InstallCredentials(self) && WolfSslStream_CreateSession(self) &&
              WolfSslStream_ApplyTlsPolicy(self) && WolfSslStream_ConfigureExpectedPeerName(self);
    if (ok)
    {
        WolfSslStream_InstallTransportCallbacks(self);
        WolfSslStream_ApplyPeerVerificationPolicy(self);
        ok = WolfSslStream_PerformHandshake(self);
    }
    if (!ok)
    {
        WolfSslStream_Close(base);
    }
    return ok;
}

/* One snapshot per connection: every later step reads the same answer, however
 * the integrator's own state moves while the handshake is in progress. */
static inline void WolfSslStream_PullProfile(struct SolidSyslogWolfSslStream* self)
{
    self->Profile = (struct SolidSyslogWolfSslProfile) {0};
    self->Config.Profile(&self->Profile, self->Config.ProfileContext);
}

/* A context per connection, on the integrator's heap hint, so nothing of one
 * connection's material survives into the next. wolfSSL_CTX_new_ex takes the
 * method over whatever it returns, so a NULL here leaks nothing. */
static inline bool WolfSslStream_BuildContext(struct SolidSyslogWolfSslStream* self)
{
    self->Context = wolfSSL_CTX_new_ex(wolfTLS_client_method_ex(self->Config.Heap), self->Config.Heap);
    bool ok = self->Context != NULL;
    if (!ok)
    {
        WolfSslStream_Report(
            SOLIDSYSLOG_SEVERITY_ERROR,
            SOLIDSYSLOG_CAT_TLS_STREAM_INIT_FAILED,
            SOLIDSYSLOG_TLS_STREAM_ERROR_CONTEXT_INIT_FAILED
        );
    }
    return ok;
}

/* Asked once per connection, onto the context and before the session is created
 * from it, so what is installed is what the session inherits. The flag is set
 * before the call rather than after it: the contract is one Release per Install
 * call whatever that call returned, which is what spares every backend a
 * rollback path of its own. */
static inline bool WolfSslStream_InstallCredentials(struct SolidSyslogWolfSslStream* self)
{
    self->Installed.TrustAnchorsInstalled = false;
    self->Installed.Fingerprints = NULL;
    self->Installed.FingerprintCount = 0U;
    self->CredentialsInstalled = true;
    bool ok = self->Config.Credentials->Install(self->Config.Credentials, self->Context, &self->Installed);
    if (ok && !WolfSslStream_PeerIsAuthorisable(&self->Installed))
    {
        WolfSslStream_Report(
            SOLIDSYSLOG_SEVERITY_ERROR,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_TLS_STREAM_ERROR_NO_PEER_AUTHORISATION
        );
        ok = false;
    }
    if (ok)
    {
        ok = WolfSslStream_FingerprintsAreUsable(&self->Installed);
    }
    return ok;
}

/* A peer is authorised by a chain to trust anchors or by a pinned certificate
 * fingerprint, and RFC 5425 section 4.2.1 makes the second sufficient on its
 * own. With neither, there is nothing to check the peer against, so the
 * connection stops rather than reaching a peer this stream cannot identify. */
static inline bool WolfSslStream_PeerIsAuthorisable(const struct SolidSyslogTlsCredentialsInstalled* installed)
{
    return installed->TrustAnchorsInstalled || (installed->FingerprintCount > 0U);
}

/* Inspected before the handshake, so a pin that cannot match is reported as
 * bad configuration rather than as a refused peer. */
static inline bool WolfSslStream_FingerprintsAreUsable(const struct SolidSyslogTlsCredentialsInstalled* installed)
{
    bool ok = true;
    enum SolidSyslogTlsFingerprintListState state =
        SolidSyslogTlsFingerprint_InspectList(installed->Fingerprints, installed->FingerprintCount);
    if (state == SOLIDSYSLOG_TLS_FINGERPRINT_LIST_MALFORMED)
    {
        WolfSslStream_Report(
            SOLIDSYSLOG_SEVERITY_ERROR,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_TLS_STREAM_ERROR_FINGERPRINT_MALFORMED
        );
        ok = false;
    }
    else if (state == SOLIDSYSLOG_TLS_FINGERPRINT_LIST_USES_SHA1)
    {
        WolfSslStream_Report(
            SOLIDSYSLOG_SEVERITY_WARNING,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_TLS_STREAM_ERROR_FINGERPRINT_SHA1
        );
    }
    else
    {
        /* Well formed, or no pins at all. */
    }
    return ok;
}

static inline bool WolfSslStream_CreateSession(struct SolidSyslogWolfSslStream* self)
{
    self->Session = wolfSSL_new(self->Context);
    bool ok = self->Session != NULL;
    if (!ok)
    {
        WolfSslStream_Report(
            SOLIDSYSLOG_SEVERITY_ERROR,
            SOLIDSYSLOG_CAT_TLS_STREAM_INIT_FAILED,
            SOLIDSYSLOG_TLS_STREAM_ERROR_SESSION_INIT_FAILED
        );
    }
    return ok;
}

/* TLS policy owned by the library, set on the session so it cannot leak into
 * anything the integrator builds from a context of their own. The floor is
 * TLS 1.2, stated here so downgrade resistance does not depend on how the
 * library was built; no ceiling is set, because RFC 9662, which updates RFC
 * 5425, requires TLS 1.3 to be preferred wherever it is implemented. The key
 * sizes are BCP 195 section 4.5. */
static inline bool WolfSslStream_ApplyTlsPolicy(struct SolidSyslogWolfSslStream* self)
{
    bool ok = (wolfSSL_SetMinVersion(self->Session, WOLFSSL_TLSV1_2) == WOLFSSL_SUCCESS) &&
              (wolfSSL_SetMinRsaKey_Sz(self->Session, (short) WOLFSSL_STREAM_MIN_RSA_KEY_BITS) == WOLFSSL_SUCCESS) &&
              (wolfSSL_SetMinEccKey_Sz(self->Session, (short) WOLFSSL_STREAM_MIN_ECC_KEY_BITS) == WOLFSSL_SUCCESS) &&
              (wolfSSL_SetMinDhKey_Sz(self->Session, (word16) WOLFSSL_STREAM_MIN_DH_KEY_BITS) == WOLFSSL_SUCCESS);
    if (!ok)
    {
        WolfSslStream_Report(
            SOLIDSYSLOG_SEVERITY_ERROR,
            SOLIDSYSLOG_CAT_TLS_STREAM_INIT_FAILED,
            SOLIDSYSLOG_TLS_STREAM_ERROR_DEFAULTS_NOT_APPLIED
        );
    }
    return ok;
}

/* The declared name is both what the certificate is checked against and what is
 * sent as the server name indication. */
static inline bool WolfSslStream_ConfigureExpectedPeerName(struct SolidSyslogWolfSslStream* self)
{
    bool ok = true;
    const char* serverName = self->Profile.ServerName;
    if (serverName == NULL)
    {
        /* No expected identity supplied - the handshake will accept any cert that
         * chains to a trusted CA, so the peer is unverified. Surface it as a
         * WARNING (still connect, preserving the closed-network case) rather than
         * swallowing the MITM-class default silently. */
        WolfSslStream_Report(
            SOLIDSYSLOG_SEVERITY_WARNING,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_TLS_STREAM_ERROR_SERVER_NAME_NOT_SET
        );
    }
    else if (serverName[0] != '\0')
    {
        ok = WolfSslStream_ApplyExpectedPeerName(self, serverName);
    }
    else
    {
        /* Empty string is the deliberate opt-out: the integrator has no name to
         * verify against (private CA) and has said so explicitly, so connect
         * chain-only without a diagnostic. */
    }
    return ok;
}

/* The declared name is both what the certificate is checked against and what is
 * sent as the server name indication. Both copy the name, so both can fail on
 * allocation; only the indication says so in its return. */
static inline bool WolfSslStream_ApplyExpectedPeerName(struct SolidSyslogWolfSslStream* self, const char* serverName)
{
    bool ok = false;
    if (WolfSslStream_DeclareExpectedPeer(self, serverName) != WOLFSSL_SUCCESS)
    {
        WolfSslStream_Report(
            SOLIDSYSLOG_SEVERITY_ERROR,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_TLS_STREAM_ERROR_SERVER_NAME_NOT_APPLIED
        );
    }
    else
    {
        int rc = wolfSSL_UseSNI(self->Session, WOLFSSL_SNI_HOST_NAME, serverName, (word16) strlen(serverName));
        ok = rc == WOLFSSL_SUCCESS;
        if (!ok)
        {
            WolfSslStream_Report(
                SOLIDSYSLOG_SEVERITY_ERROR,
                SOLIDSYSLOG_CAT_BAD_CONFIG,
                WolfSslStream_InitDetail(rc, SOLIDSYSLOG_TLS_STREAM_ERROR_SERVER_NAME_NOT_APPLIED)
            );
        }
    }
    return ok;
}

/* wolfSSL keeps a name and an address apart: a name is matched against the
 * dNSName entries or the Common Name, and validated as a hostname when it is
 * declared; an address only ever against the iPAddress entries. The contract
 * wants the same split, so which call takes the declaration is decided here
 * rather than left to the library's hostname rules to refuse. */
static inline int WolfSslStream_DeclareExpectedPeer(struct SolidSyslogWolfSslStream* self, const char* serverName)
{
    return WolfSslStream_IsAddressLiteral(serverName) ? wolfSSL_check_ip_address(self->Session, serverName)
                                                      : wolfSSL_check_domain_name(self->Session, serverName);
}

/* A colon is only ever in an IPv6 literal, and a name made of nothing but
 * digits and dots is an IPv4 one - no hostname has an all-numeric last label.
 * What the literal is worth is the library's to judge against the certificate. */
static inline bool WolfSslStream_IsAddressLiteral(const char* name)
{
    bool onlyDigitsAndDots = true;
    bool hasColon = false;
    for (size_t index = 0U; name[index] != '\0'; index++)
    {
        if (name[index] == ':')
        {
            hasColon = true;
        }
        if (!WolfSslStream_IsDigitOrDot(name[index]))
        {
            onlyDigitsAndDots = false;
        }
    }
    return hasColon || onlyDigitsAndDots;
}

static inline bool WolfSslStream_IsDigitOrDot(char c)
{
    return ((c >= '0') && (c <= '9')) || (c == '.');
}

/* wolfSSL names an allocation failure in its return, so say so rather than
 * report the phase's own fault - which would send an integrator to their
 * configuration or to the peer, and the allocator is neither. */
static inline enum SolidSyslogTlsStreamErrors WolfSslStream_InitDetail(
    int rc,
    enum SolidSyslogTlsStreamErrors otherwise
)
{
    return (rc == MEMORY_E) ? SOLIDSYSLOG_TLS_STREAM_ERROR_LIBRARY_OUT_OF_MEMORY : otherwise;
}

static inline void WolfSslStream_InstallTransportCallbacks(struct SolidSyslogWolfSslStream* self)
{
    wolfSSL_SSLSetIORecv(self->Session, WolfSslStream_IoRecv);
    wolfSSL_SSLSetIOSend(self->Session, WolfSslStream_IoSend);
    wolfSSL_SetIOReadCtx(self->Session, self);
    wolfSSL_SetIOWriteCtx(self->Session, self);
}

/* The peer must present a certificate and it must verify; the callback records
 * what the library found wrong, so the refusal can name the check. */
static inline void WolfSslStream_ApplyPeerVerificationPolicy(struct SolidSyslogWolfSslStream* self)
{
    wolfSSL_set_verify(
        self->Session,
        WOLFSSL_VERIFY_PEER | WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT,
        WolfSslStream_VerifyPeer
    );
    wolfSSL_SetCertCbCtx(self->Session, self);
}

/* Called for each check a certificate in the chain fails - and, on a build
 * with WOLFSSL_ALWAYS_VERIFY_CB, for each certificate that passes. Nothing is
 * overridden: the library's verdict stands, and the stream only records which
 * check it was, because the handshake's return carries one code and not
 * necessarily the one the contract puts first. */
static int WolfSslStream_VerifyPeer(int preverify, WOLFSSL_X509_STORE_CTX* store)
{
    struct SolidSyslogWolfSslStream* self = (struct SolidSyslogWolfSslStream*) store->userCtx;
    int verdict = preverify;
    if ((self->Installed.FingerprintCount > 0U) && !self->Installed.TrustAnchorsInstalled &&
        (store->error == ASN_NO_SIGNER_E))
    {
        verdict = WolfSslStream_LeafMatchesAPin(self, store) ? 1 : 0;
        if (verdict == 0)
        {
            self->RefusedChecks |= WOLFSSL_STREAM_CHECK_FINGERPRINT;
        }
    }
    else
    {
        self->RefusedChecks |= WolfSslStream_CheckFailedFor(store->error);
    }
    return verdict;
}

/* A pin is the digest of the leaf. wolfSSL hands the callback the whole chain
 * the peer presented, leaf first, whichever certificate it is calling back
 * for. */
static inline bool WolfSslStream_LeafMatchesAPin(struct SolidSyslogWolfSslStream* self, WOLFSSL_X509_STORE_CTX* store)
{
    enum SolidSyslogTlsAuthorisation verdict = SolidSyslogTlsFingerprint_Authorise(
        self->Installed.Fingerprints,
        self->Installed.FingerprintCount,
        WolfSslStream_DigestCertificate,
        &store->certs[0]
    );
    return verdict == SOLIDSYSLOG_TLS_AUTHORISATION_MATCHED;
}

static bool WolfSslStream_DigestCertificate(
    void* context,
    enum SolidSyslogTlsHashAlgorithm algorithm,
    uint8_t* digest,
    size_t* length
)
{
    const WOLFSSL_BUFFER_INFO* certificate = (const WOLFSSL_BUFFER_INFO*) context;
    enum wc_HashType type = WolfSslStream_HashTypeFor(algorithm);
    int size = wc_HashGetDigestSize(type);
    bool ok = (size > 0) && (wc_Hash(type, certificate->buffer, certificate->length, digest, (word32) size) == 0);
    if (ok)
    {
        *length = (size_t) size;
    }
    return ok;
}

static inline enum wc_HashType WolfSslStream_HashTypeFor(enum SolidSyslogTlsHashAlgorithm algorithm)
{
    enum wc_HashType type = WC_HASH_TYPE_SHA256;
    if (algorithm == SOLIDSYSLOG_TLS_HASH_SHA1)
    {
        type = WC_HASH_TYPE_SHA;
    }
    return type;
}

/* Every code the stream does not name individually is a certificate that
 * fails path validation for a reason the integrator's next step - replace the
 * certificate, not the network - does not depend on, so it reads as untrusted. */
static inline uint32_t WolfSslStream_CheckFailedFor(int error)
{
    uint32_t check = 0U;
    if (error == 0)
    {
        /* The certificate passed; nothing to record. */
    }
    else if ((error == DOMAIN_NAME_MISMATCH) || (error == IPADDR_MISMATCH))
    {
        check = WOLFSSL_STREAM_CHECK_NAME;
    }
    else if (error == ASN_AFTER_DATE_E)
    {
        check = WOLFSSL_STREAM_CHECK_NOT_AFTER;
    }
    else if (error == ASN_BEFORE_DATE_E)
    {
        check = WOLFSSL_STREAM_CHECK_NOT_BEFORE;
    }
    else
    {
        check = WOLFSSL_STREAM_CHECK_CHAIN_TRUST;
    }
    return check;
}

/* Bridges the integrator-installed getter (or the Null Object substituted at
 * config-copy time) to the bounded handshake deadline. Invoked at the start
 * of each handshake attempt so runtime-tunable values take effect on the next
 * reconnect. */
static inline uint32_t WolfSslStream_ResolveHandshakeTimeoutMs(struct SolidSyslogWolfSslStream* self)
{
    return self->Config.GetHandshakeTimeoutMs(self->Config.HandshakeTimeoutContext);
}

/* Drive wolfSSL_connect to completion under non-blocking transport. Each call
 * may want more I/O while waiting for the multi-RTT handshake to progress; the
 * loop sleeps briefly between attempts (avoiding a busy spin) until either the
 * handshake completes, hits a hard error, or the bounded budget expires. Each
 * non-success exit emits a distinct code so the integrator can tell rejection
 * from timeout. */
static inline bool WolfSslStream_PerformHandshake(struct SolidSyslogWolfSslStream* self)
{
    uint32_t budgetMs = WolfSslStream_ResolveHandshakeTimeoutMs(self);
    uint32_t totalSleptMs = 0U;
    bool result = false;
    bool done = false;

    while (!done)
    {
        int rc = wolfSSL_connect(self->Session);
        int error = wolfSSL_get_error(self->Session, rc);
        if (rc == WOLFSSL_SUCCESS)
        {
            result = true;
            done = true;
        }
        else if (!WolfSslStream_IsRetryableHandshakeError(error))
        {
            WolfSslStream_ReportHandshakeFailure(
                self,
                SOLIDSYSLOG_SEVERITY_ERROR,
                WolfSslStream_InitDetail(error, WolfSslStream_RefusalDetail(self))
            );
            done = true;
        }
        else if (WolfSslStream_IsHandshakeBudgetExhausted(totalSleptMs, budgetMs))
        {
            WolfSslStream_ReportHandshakeFailure(
                self,
                SOLIDSYSLOG_SEVERITY_WARNING,
                SOLIDSYSLOG_TLS_STREAM_ERROR_HANDSHAKE_TIMEOUT
            );
            done = true;
        }
        else
        {
            uint32_t pollMs = WolfSslStream_HandshakePollMs(totalSleptMs, budgetMs);
            self->Config.Sleep(pollMs);
            totalSleptMs += pollMs;
        }
    }
    return result;
}

static inline bool WolfSslStream_IsRetryableHandshakeError(int error)
{
    return (error == WOLFSSL_ERROR_WANT_READ) || (error == WOLFSSL_ERROR_WANT_WRITE);
}

static inline bool WolfSslStream_IsHandshakeBudgetExhausted(uint32_t totalSleptMs, uint32_t budgetMs)
{
    return totalSleptMs >= budgetMs;
}

/* The poll interval, or what is left of the budget where that is less, so the
 * sleeps requested never add up to more than the budget. How long each sleep
 * really takes, and the handshake itself, can still carry the attempt past it.
 * Called only while budget remains. */
static inline uint32_t WolfSslStream_HandshakePollMs(uint32_t totalSleptMs, uint32_t budgetMs)
{
    uint32_t pollMs = (uint32_t) HANDSHAKE_POLL_INTERVAL_MILLISECONDS;
    uint32_t remainingMs = budgetMs - totalSleptMs;
    if (remainingMs < pollMs)
    {
        pollMs = remainingMs;
    }
    return pollMs;
}

/* The checks accumulate, so a compound verdict resolves by precedence: an
 * untrusted chain is reported ahead of anything the certificate says about
 * itself, because a certificate no anchor vouches for is not made acceptable by
 * the name or the dates it carries; and the name ahead of the dates, as the
 * contract orders them. With no check recorded the handshake was rejected by
 * the peer or the protocol, and no check of ours names it. */
static inline enum SolidSyslogTlsStreamErrors WolfSslStream_RefusalDetail(const struct SolidSyslogWolfSslStream* self)
{
    enum SolidSyslogTlsStreamErrors detail = SOLIDSYSLOG_TLS_STREAM_ERROR_HANDSHAKE_REJECTED;
    if (WolfSslStream_Failed(self, WOLFSSL_STREAM_CHECK_FINGERPRINT))
    {
        detail = SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_FINGERPRINT_MISMATCHED;
    }
    else if (WolfSslStream_Failed(self, WOLFSSL_STREAM_CHECK_CHAIN_TRUST))
    {
        detail = SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_UNTRUSTED;
    }
    else if (WolfSslStream_Failed(self, WOLFSSL_STREAM_CHECK_NAME))
    {
        detail = SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_NAME_MISMATCHED;
    }
    else if (WolfSslStream_Failed(self, WOLFSSL_STREAM_CHECK_NOT_AFTER))
    {
        detail = SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_EXPIRED;
    }
    else if (WolfSslStream_Failed(self, WOLFSSL_STREAM_CHECK_NOT_BEFORE))
    {
        detail = SOLIDSYSLOG_TLS_STREAM_ERROR_PEER_CERTIFICATE_NOT_YET_VALID;
    }
    else
    {
        /* Nothing recorded: the detail already holds the plain rejection. */
    }
    return detail;
}

static inline bool WolfSslStream_Failed(const struct SolidSyslogWolfSslStream* self, uint32_t check)
{
    return (self->RefusedChecks & check) != 0U;
}

/* TLS-level write failure means the session state is unrecoverable - close
 * so the StreamSender reconnect path runs on the next tick. Fail-fast is the
 * contract every TLS stream adapter honours. */
static inline bool WolfSslStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size)
{
    struct SolidSyslogWolfSslStream* self = WolfSslStream_SelfFromBase(base);
    bool ok = false;
    if (WolfSslStream_SessionIsStillOpen(base))
    {
        int rc = wolfSSL_write(self->Session, buffer, (int) size);
        ok = (rc > 0) && ((size_t) rc == size);
        if (!ok)
        {
            WolfSslStream_Close(base);
        }
    }
    return ok;
}

/* wolfSSL_write does not look at what the peer has sent, so a collector that
 * has closed the session - a close_notify and a FIN - still has the record
 * accepted, and the transport beneath may not see the FIN behind the unread
 * alert. Read what is waiting first: an alert or a closed transport ends the
 * session here, and Read closes it. A collector sends no application data, so
 * any that arrives is discarded; a peer that keeps sending is plainly still
 * there, and the record goes out once the bound is reached. */
static bool WolfSslStream_SessionIsStillOpen(struct SolidSyslogStream* base)
{
    unsigned char discard[WOLFSSL_STREAM_DISCARD_BYTES];
    SolidSyslogSsize pending = WolfSslStream_Read(base, discard, sizeof(discard));
    uint32_t reads = 1U;
    while ((pending > 0) && (reads < WOLFSSL_STREAM_MAX_READS_BEFORE_WRITE))
    {
        pending = WolfSslStream_Read(base, discard, sizeof(discard));
        reads++;
    }
    return pending >= 0;
}

/* wolfSSL_read has two distinct outcomes worth keeping straight:
 *   1. Steady-state read: bytes available -> positive count; nothing to read
 *      right now -> WANT_READ -> return 0, mirroring the transport contract.
 *   2. Any other return (close_notify, a fatal alert, a hard transport error,
 *      a session that is not open) is fatal under fail-fast semantics - close
 *      internally; the caller reopens and store-and-forward replays. */
static inline SolidSyslogSsize WolfSslStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size)
{
    struct SolidSyslogWolfSslStream* self = WolfSslStream_SelfFromBase(base);
    int rc = wolfSSL_read(self->Session, buffer, (int) size);
    SolidSyslogSsize result = -1;
    if (rc > 0)
    {
        result = (SolidSyslogSsize) rc;
    }
    else if (wolfSSL_get_error(self->Session, rc) == WOLFSSL_ERROR_WANT_READ)
    {
        result = 0;
    }
    else
    {
        WolfSslStream_Close(base);
    }
    return result;
}

static int WolfSslStream_IoSend(WOLFSSL* ssl, char* buf, int sz, void* ctx)
{
    struct SolidSyslogWolfSslStream* self = (struct SolidSyslogWolfSslStream*) ctx;
    (void) ssl;
    return SolidSyslogStream_Send(self->Config.Transport, buf, (size_t) sz) ? sz : WOLFSSL_CBIO_ERR_GENERAL;
}

static int WolfSslStream_IoRecv(WOLFSSL* ssl, char* buf, int sz, void* ctx)
{
    struct SolidSyslogWolfSslStream* self = (struct SolidSyslogWolfSslStream*) ctx;
    (void) ssl;
    SolidSyslogSsize n = SolidSyslogStream_Read(self->Config.Transport, buf, (size_t) sz);
    int result = WOLFSSL_CBIO_ERR_GENERAL;
    if (n > 0)
    {
        result = (int) n;
    }
    else if (n == 0)
    {
        result = WOLFSSL_CBIO_ERR_WANT_READ;
    }
    else
    {
        /* Transport-level error; a general failure aborts the handshake / read. */
    }
    return result;
}
