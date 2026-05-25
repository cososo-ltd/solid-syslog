#include "SolidSyslogMbedTlsStream.h"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ssl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogMbedTlsStreamErrors.h"
#include "SolidSyslogMbedTlsStreamPrivate.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTunables.h"

enum
{
    HANDSHAKE_POLL_INTERVAL_MILLISECONDS = 1
};

struct SolidSyslogAddress;

static uint32_t MbedTlsStream_NullHandshakeTimeoutGetter(void* context);
static inline bool MbedTlsStream_ConfigProvidesHandshakeGetter(const struct SolidSyslogMbedTlsStreamConfig* config);
static inline uint32_t MbedTlsStream_ResolveHandshakeTimeoutMs(struct SolidSyslogMbedTlsStream* self);
static inline struct SolidSyslogMbedTlsStream* MbedTlsStream_SelfFromBase(struct SolidSyslogStream* base);
static inline bool MbedTlsStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);
static inline bool MbedTlsStream_ApplySslConfigDefaults(struct SolidSyslogMbedTlsStream* self);
static inline void MbedTlsStream_ApplyTlsPolicy(struct SolidSyslogMbedTlsStream* self);
static inline bool MbedTlsStream_BindContextToConfig(struct SolidSyslogMbedTlsStream* self);
static inline bool MbedTlsStream_ConfigureExpectedHostname(struct SolidSyslogMbedTlsStream* self);
static inline void MbedTlsStream_InstallTransportCallbacks(struct SolidSyslogMbedTlsStream* self);
static inline bool MbedTlsStream_PerformHandshake(struct SolidSyslogMbedTlsStream* self);
static inline bool MbedTlsStream_IsRetryableHandshakeRc(int rc);
static inline bool MbedTlsStream_IsHandshakeBudgetExhausted(uint32_t totalSleptMs, uint32_t budgetMs);
static inline bool MbedTlsStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size);
static inline SolidSyslogSsize MbedTlsStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size);
static inline void MbedTlsStream_Close(struct SolidSyslogStream* base);
static int MbedTlsStream_BioSend(void* ctx, const unsigned char* buf, size_t len);
static int MbedTlsStream_BioRecv(void* ctx, unsigned char* buf, size_t len);

void MbedTlsStream_Initialise(struct SolidSyslogStream* base, const struct SolidSyslogMbedTlsStreamConfig* config)
{
    struct SolidSyslogMbedTlsStream* self = MbedTlsStream_SelfFromBase(base);
    self->Base.Open = MbedTlsStream_Open;
    self->Base.Send = MbedTlsStream_Send;
    self->Base.Read = MbedTlsStream_Read;
    self->Base.Close = MbedTlsStream_Close;
    self->Config = *config;
    if (MbedTlsStream_ConfigProvidesHandshakeGetter(config) == false)
    {
        /* Substitute the Null Object so the bounded-handshake loop has a
         * single code path regardless of whether the integrator wired
         * runtime tuning. */
        self->Config.GetHandshakeTimeoutMs = MbedTlsStream_NullHandshakeTimeoutGetter;
        self->Config.HandshakeTimeoutContext = NULL;
    }
    /* Eager init so mbedtls_*_free in Close is always safe — whether Open
     * was ever reached, whether it succeeded, or whether Close is being
     * called twice in a row. mbedTLS guarantees a freed struct is left in
     * the same zeroed state an init produces, so re-Open after Close also
     * works without re-init. */
    mbedtls_ssl_init(&self->SslContext);
    mbedtls_ssl_config_init(&self->SslConfig);
}

/* Null Object substituted in Initialise when the integrator does not install a
 * getter — returns the compile-time tunable so the bounded-handshake path is a
 * single code path regardless of whether the integrator wired runtime tuning. */
static uint32_t MbedTlsStream_NullHandshakeTimeoutGetter(void* context)
{
    (void) context;
    return (uint32_t) SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS;
}

static inline bool MbedTlsStream_ConfigProvidesHandshakeGetter(const struct SolidSyslogMbedTlsStreamConfig* config)
{
    return (config != NULL) && (config->GetHandshakeTimeoutMs != NULL);
}

/* Bridges the integrator-installed getter (or the Null Object substituted at
 * config-copy time) to the bounded handshake deadline. Invoked at the start
 * of each handshake attempt so runtime-tunable values take effect on the next
 * reconnect. */
static inline uint32_t MbedTlsStream_ResolveHandshakeTimeoutMs(struct SolidSyslogMbedTlsStream* self)
{
    return self->Config.GetHandshakeTimeoutMs(self->Config.HandshakeTimeoutContext);
}

static inline struct SolidSyslogMbedTlsStream* MbedTlsStream_SelfFromBase(struct SolidSyslogStream* base)
{
    return (struct SolidSyslogMbedTlsStream*) base;
}

void MbedTlsStream_Cleanup(struct SolidSyslogStream* base)
{
    /* Mirror the OpenSSL TlsStream pattern: an integrator who destroys a
     * still-Open stream must not leak the underlying TLS state. */
    MbedTlsStream_Close(base);
    /* Overwrite the abstract base with the shared NullStream vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullStream_Get();
}

/* Idempotent: a previous Close left the structs in mbedTLS's freed-equivalent
 * (zeroed) state, so close_notify sees conf == NULL and returns harmlessly,
 * and the *_free calls are no-ops on already-freed structs. Transport Close
 * is itself idempotent on every Stream impl. */
static inline void MbedTlsStream_Close(struct SolidSyslogStream* base)
{
    struct SolidSyslogMbedTlsStream* self = MbedTlsStream_SelfFromBase(base);
    (void) mbedtls_ssl_close_notify(&self->SslContext);
    mbedtls_ssl_free(&self->SslContext);
    mbedtls_ssl_config_free(&self->SslConfig);
    SolidSyslogStream_Close(self->Config.Transport);
}

static inline bool MbedTlsStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr)
{
    struct SolidSyslogMbedTlsStream* self = MbedTlsStream_SelfFromBase(base);
    bool ok = SolidSyslogStream_Open(self->Config.Transport, addr) && MbedTlsStream_ApplySslConfigDefaults(self);
    if (ok)
    {
        MbedTlsStream_ApplyTlsPolicy(self);
        ok = MbedTlsStream_BindContextToConfig(self) && MbedTlsStream_ConfigureExpectedHostname(self);
    }
    if (ok)
    {
        MbedTlsStream_InstallTransportCallbacks(self);
        ok = MbedTlsStream_PerformHandshake(self);
    }
    if (!ok)
    {
        MbedTlsStream_Close(base);
    }
    return ok;
}

static inline bool MbedTlsStream_ApplySslConfigDefaults(struct SolidSyslogMbedTlsStream* self)
{
    bool ok = mbedtls_ssl_config_defaults(
                  &self->SslConfig,
                  MBEDTLS_SSL_IS_CLIENT,
                  MBEDTLS_SSL_TRANSPORT_STREAM,
                  MBEDTLS_SSL_PRESET_DEFAULT
              ) == 0;
    if (!ok)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &MbedTlsStreamErrorSource,
            (uint8_t) MBEDTLSSTREAM_ERROR_DEFAULTS_NOT_APPLIED
        );
    }
    return ok;
}

/* TLS policy owned by the library — set per-ssl_config so it cannot leak
 * into the integrator's other ssl_configs (per coexistence contract). */
static inline void MbedTlsStream_ApplyTlsPolicy(struct SolidSyslogMbedTlsStream* self)
{
    mbedtls_ssl_conf_authmode(&self->SslConfig, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&self->SslConfig, self->Config.CaChain, NULL);
    mbedtls_ssl_conf_rng(&self->SslConfig, mbedtls_ctr_drbg_random, self->Config.Rng);
    if ((self->Config.ClientCertChain != NULL) && (self->Config.ClientKey != NULL))
    {
        (void) mbedtls_ssl_conf_own_cert(&self->SslConfig, self->Config.ClientCertChain, self->Config.ClientKey);
    }
}

static inline bool MbedTlsStream_BindContextToConfig(struct SolidSyslogMbedTlsStream* self)
{
    bool ok = mbedtls_ssl_setup(&self->SslContext, &self->SslConfig) == 0;
    if (!ok)
    {
        SolidSyslog_Error(
            SOLIDSYSLOG_SEVERITY_ERROR,
            &MbedTlsStreamErrorSource,
            (uint8_t) MBEDTLSSTREAM_ERROR_SESSION_INIT_FAILED
        );
    }
    return ok;
}

static inline bool MbedTlsStream_ConfigureExpectedHostname(struct SolidSyslogMbedTlsStream* self)
{
    bool ok = true;
    if (self->Config.ServerName != NULL)
    {
        ok = mbedtls_ssl_set_hostname(&self->SslContext, self->Config.ServerName) == 0;
        if (!ok)
        {
            SolidSyslog_Error(
                SOLIDSYSLOG_SEVERITY_ERROR,
                &MbedTlsStreamErrorSource,
                (uint8_t) MBEDTLSSTREAM_ERROR_SERVER_NAME_NOT_SET
            );
        }
    }
    return ok;
}

static inline void MbedTlsStream_InstallTransportCallbacks(struct SolidSyslogMbedTlsStream* self)
{
    mbedtls_ssl_set_bio(&self->SslContext, self, MbedTlsStream_BioSend, MbedTlsStream_BioRecv, NULL);
}

/* Drive mbedtls_ssl_handshake to completion under non-blocking transport.
 * Each call may return WANT_READ/WANT_WRITE while waiting for the multi-RTT
 * handshake to progress; we sleep briefly between attempts (avoiding a busy
 * spin) until either the handshake completes, hits a hard error, or the
 * bounded budget expires. Each non-success exit emits a distinct
 * protocol-level error code so the integrator can tell rejection from
 * timeout. Same shape as OpenSSL's TlsStream_PerformHandshake. */
static inline bool MbedTlsStream_PerformHandshake(struct SolidSyslogMbedTlsStream* self)
{
    uint32_t budgetMs = MbedTlsStream_ResolveHandshakeTimeoutMs(self);
    uint32_t totalSleptMs = 0;
    bool result = false;
    bool done = false;

    while (!done)
    {
        int rc = mbedtls_ssl_handshake(&self->SslContext);
        if (rc == 0)
        {
            result = true;
            done = true;
        }
        else if (!MbedTlsStream_IsRetryableHandshakeRc(rc))
        {
            SolidSyslog_Error(
                SOLIDSYSLOG_SEVERITY_ERROR,
                &MbedTlsStreamErrorSource,
                (uint8_t) MBEDTLSSTREAM_ERROR_HANDSHAKE_REJECTED
            );
            done = true;
        }
        else if (MbedTlsStream_IsHandshakeBudgetExhausted(totalSleptMs, budgetMs))
        {
            SolidSyslog_Error(
                SOLIDSYSLOG_SEVERITY_ERROR,
                &MbedTlsStreamErrorSource,
                (uint8_t) MBEDTLSSTREAM_ERROR_HANDSHAKE_TIMEOUT
            );
            done = true;
        }
        else
        {
            self->Config.Sleep(HANDSHAKE_POLL_INTERVAL_MILLISECONDS);
            totalSleptMs += (uint32_t) HANDSHAKE_POLL_INTERVAL_MILLISECONDS;
        }
    }
    return result;
}

static inline bool MbedTlsStream_IsRetryableHandshakeRc(int rc)
{
    return (rc == MBEDTLS_ERR_SSL_WANT_READ) || (rc == MBEDTLS_ERR_SSL_WANT_WRITE);
}

static inline bool MbedTlsStream_IsHandshakeBudgetExhausted(uint32_t totalSleptMs, uint32_t budgetMs)
{
    return totalSleptMs >= budgetMs;
}

static int MbedTlsStream_BioSend(void* ctx, const unsigned char* buf, size_t len)
{
    struct SolidSyslogMbedTlsStream* self = (struct SolidSyslogMbedTlsStream*) ctx;
    return SolidSyslogStream_Send(self->Config.Transport, buf, len) ? (int) len : -1;
}

/* Translate the non-blocking transport's Read contract into mbedTLS's BIO
 * recv contract:
 *   transport > 0 → bytes available, return the same positive count.
 *   transport = 0 → would-block. Must return MBEDTLS_ERR_SSL_WANT_READ so
 *                  the handshake retry loop polls; returning 0 or -1 would
 *                  abort the handshake on the first non-blocking read. */
static int MbedTlsStream_BioRecv(void* ctx, unsigned char* buf, size_t len)
{
    struct SolidSyslogMbedTlsStream* self = (struct SolidSyslogMbedTlsStream*) ctx;
    SolidSyslogSsize n = SolidSyslogStream_Read(self->Config.Transport, buf, len);
    int result = -1;
    if (n > 0)
    {
        result = (int) n;
    }
    else if (n == 0)
    {
        result = MBEDTLS_ERR_SSL_WANT_READ;
    }
    else
    {
        /* n < 0 — transport-level error; keep result = -1 to signal a
           hard failure to mbedTLS so the handshake / read aborts. */
    }
    return result;
}

/* TLS-level write failure means the session state is unrecoverable — close
 * so the StreamSender reconnect path runs on the next tick. Mirrors the
 * OpenSSL TlsStream_Send fail-fast contract. */
static inline bool MbedTlsStream_Send(struct SolidSyslogStream* base, const void* buffer, size_t size)
{
    struct SolidSyslogMbedTlsStream* self = MbedTlsStream_SelfFromBase(base);
    int rc = mbedtls_ssl_write(&self->SslContext, (const unsigned char*) buffer, size);
    bool ok = (rc > 0) && ((size_t) rc == size);
    if (!ok)
    {
        MbedTlsStream_Close(base);
    }
    return ok;
}

/* mbedtls_ssl_read has two distinct outcomes worth keeping straight:
 *   1. Steady-state read: bytes available → positive count; nothing to read
 *      right now → WANT_READ → return 0, mirroring the transport contract.
 *   2. Any other negative return (alerts, renegotiation surfacing as
 *      WANT_WRITE, hard transport error) is fatal under fail-fast semantics
 *      — close internally; the caller reopens and store-and-forward replays.
 * Same shape as the OpenSSL TlsStream_Read. */
static inline SolidSyslogSsize MbedTlsStream_Read(struct SolidSyslogStream* base, void* buffer, size_t size)
{
    struct SolidSyslogMbedTlsStream* self = MbedTlsStream_SelfFromBase(base);
    int rc = mbedtls_ssl_read(&self->SslContext, (unsigned char*) buffer, size);
    SolidSyslogSsize result = -1;
    if (rc > 0)
    {
        result = (SolidSyslogSsize) rc;
    }
    else if (rc == MBEDTLS_ERR_SSL_WANT_READ)
    {
        result = 0;
    }
    else
    {
        MbedTlsStream_Close(base);
    }
    return result;
}
