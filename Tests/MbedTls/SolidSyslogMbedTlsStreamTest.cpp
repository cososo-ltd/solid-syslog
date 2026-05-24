#include "CppUTest/TestHarness.h"

extern "C"
{
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ssl.h>

#include "ErrorHandlerFake.h"
#include "MbedTlsFake.h"
#include "SolidSyslogMbedTlsStream.h"
#include "SolidSyslogMbedTlsStreamErrors.h"
#include "SolidSyslogPrival.h"
#include "AddressFake.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"
#include "StreamFake.h"
}

#include "TestUtils.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings ONCE/NEVER/TWICE into scope for CALLED_FUNCTION / CALLED_FAKE

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while) -- macro preserves __FILE__/__LINE__ in failure output; do-while wraps the multi-statement body for safe single-statement use
#define CHECK_OPEN_UNWOUND_WITH_ERROR(transport, expectedCode)                    \
    do                                                                            \
    {                                                                             \
        LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));                     \
        LONGS_EQUAL(1, MbedTlsFake_SslFreeCallCount());                           \
        LONGS_EQUAL(1, MbedTlsFake_SslConfigFreeCallCount());                     \
        CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);                               \
        POINTERS_EQUAL(&MbedTlsStreamErrorSource, ErrorHandlerFake_LastSource()); \
        UNSIGNED_LONGS_EQUAL((expectedCode), ErrorHandlerFake_LastCode());        \
        LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity()); \
    } while (0)
// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

static int NoOpSleepCallCount;
static int g_lastSleepMs;

static void NoOpSleep(int milliseconds)
{
    NoOpSleepCallCount++;
    g_lastSleepMs = milliseconds;
}

// clang-format off
TEST_GROUP(SolidSyslogMbedTlsStream)
{
    struct SolidSyslogStream*            transport = nullptr;
    struct SolidSyslogStream*            handle    = nullptr;
    struct SolidSyslogMbedTlsStreamConfig config   = {};
    struct SolidSyslogAddress*           addr      = nullptr;

    void setup() override
    {
        MbedTlsFake_Reset();
        ErrorHandlerFake_Install(nullptr);
        NoOpSleepCallCount = 0;
        g_lastSleepMs = 0;
        transport = StreamFake_Create();
        config.Transport = transport;
        config.Sleep = NoOpSleep;
        handle = SolidSyslogMbedTlsStream_Create(&config);
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        addr = AddressFake_Get();
    }

    void teardown() override
    {
        SolidSyslogMbedTlsStream_Destroy(handle);
        StreamFake_Destroy(transport);
    }

    /* Tests needing config tweaks (CaChain, Rng, ServerName, …) call this
     * to release setup()'s pool slot, mutate `config`, then re-Create.
     * Fully resets the fixture (transport, MbedTls fake counters, error
     * handler) so the test body observes counts from this Open onwards
     * only — matters for assertions like CHECK_OPEN_UNWOUND_WITH_ERROR
     * that pin counts at == 1. */
    void ReCreateHandleWithUpdatedConfig()
    {
        SolidSyslogMbedTlsStream_Destroy(handle);
        StreamFake_Destroy(transport);
        MbedTlsFake_Reset();
        ErrorHandlerFake_Install(nullptr);
        transport = StreamFake_Create();
        config.Transport = transport;
        handle = SolidSyslogMbedTlsStream_Create(&config);
    }

    /* Arrange mbedtls_ssl_handshake to first emit `wantError`, then succeed on
     * the next call — exercises the bounded handshake retry loop's progress
     * path. mbedTLS returns the error code directly (no get_error indirection). */
    static void ArrangeHandshakeRetryThenSucceed(int wantError)
    {
        int seq[] = {wantError, 0};
        MbedTlsFake_SetSslHandshakeReturnSequence(seq, 2);
    }

    /* Arrange mbedtls_ssl_handshake to fail with `errorCode` on every call —
     * used both for the persistent-WANT (budget-exhausted) and hard-error paths. */
    static void ArrangePersistentHandshakeError(int errorCode)
    {
        int seq[] = {errorCode};
        MbedTlsFake_SetSslHandshakeReturnSequence(seq, 1);
    }
};

// clang-format on

TEST(SolidSyslogMbedTlsStream, OpenDelegatesToInjectedTransport)
{
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, StreamFake_OpenCallCount(transport));
    POINTERS_EQUAL(addr, StreamFake_LastOpenAddr(transport));
}

TEST(SolidSyslogMbedTlsStream, CreateInitialisesSslConfigForSafeFree)
{
    /* Init happens eagerly in Create (via MbedTlsStream_Initialise) so the
     * symmetric *_free in Close is always safe — whether Open was reached,
     * whether it succeeded, or whether Close is called more than once. */
    LONGS_EQUAL(1, MbedTlsFake_SslConfigInitCallCount());
}

TEST(SolidSyslogMbedTlsStream, OpenAppliesClientStreamDefaultsToSslConfig)
{
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, MbedTlsFake_SslConfigDefaultsCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslConfigInitArg(), MbedTlsFake_LastSslConfigDefaultsConfigArg());
    LONGS_EQUAL(MBEDTLS_SSL_IS_CLIENT, MbedTlsFake_LastSslConfigDefaultsEndpoint());
    LONGS_EQUAL(MBEDTLS_SSL_TRANSPORT_STREAM, MbedTlsFake_LastSslConfigDefaultsTransport());
    LONGS_EQUAL(MBEDTLS_SSL_PRESET_DEFAULT, MbedTlsFake_LastSslConfigDefaultsPreset());
}

TEST(SolidSyslogMbedTlsStream, CreateInitialisesSslContextForSafeFree)
{
    /* Same eager-init invariant as the SslConfig case above. */
    LONGS_EQUAL(1, MbedTlsFake_SslInitCallCount());
}

TEST(SolidSyslogMbedTlsStream, OpenBindsContextToConfig)
{
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, MbedTlsFake_SslSetupCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslInitArg(), MbedTlsFake_LastSslSetupContextArg());
    POINTERS_EQUAL(MbedTlsFake_LastSslConfigInitArg(), MbedTlsFake_LastSslSetupConfigArg());
}

TEST(SolidSyslogMbedTlsStream, OpenWiresBioWithNonNullSendRecvAndNullRecvTimeout)
{
    /* mbedTLS's set_bio takes both a recv and a recv_timeout callback;
     * we install the former (non-blocking would-block via WANT_READ) and
     * leave the latter NULL since we manage timeouts via PerformHandshake's
     * Sleep-based budget rather than mbedTLS's internal timer. */
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, MbedTlsFake_SslSetBioCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslInitArg(), MbedTlsFake_LastSslSetBioContextArg());
    CHECK(MbedTlsFake_LastSslSetBioPBioArg() != nullptr);
    CHECK(MbedTlsFake_LastSslSetBioSendCallback() != nullptr);
    CHECK(MbedTlsFake_LastSslSetBioRecvCallback() != nullptr);
    POINTERS_EQUAL(nullptr, (void*) MbedTlsFake_LastSslSetBioRecvTimeoutCallback());
}

TEST(SolidSyslogMbedTlsStream, OpenDrivesHandshakeOnTheSslContext)
{
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, MbedTlsFake_SslHandshakeCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslInitArg(), MbedTlsFake_LastSslHandshakeArg());
}

TEST(SolidSyslogMbedTlsStream, OpenReturnsTrueWhenHandshakeSucceeds)
{
    MbedTlsFake_SetSslHandshakeReturn(0);

    CHECK_TRUE(SolidSyslogStream_Open(handle, addr));
}

TEST(SolidSyslogMbedTlsStream, OpenReturnsFalseWhenHandshakeFails)
{
    MbedTlsFake_SetSslHandshakeReturn(-1);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));
}

/* -------------------------------------------------------------------------
 * Bounded handshake retry loop. mbedtls_ssl_handshake under non-blocking
 * transport will emit MBEDTLS_ERR_SSL_WANT_READ / WANT_WRITE between RTTs;
 * the loop must drive it to completion within a bounded budget so a wedged
 * peer doesn't burn the service thread indefinitely. Mirrors the OpenSSL
 * TlsStream pattern (Tests/SolidSyslogTlsStreamTest.cpp).
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogMbedTlsStream, OpenRetriesHandshakeOnWantRead)
{
    ArrangeHandshakeRetryThenSucceed(MBEDTLS_ERR_SSL_WANT_READ);

    CHECK_TRUE(SolidSyslogStream_Open(handle, addr));
    CALLED_FAKE(MbedTlsFake_SslHandshake, TWICE);
}

TEST(SolidSyslogMbedTlsStream, OpenSleepsBetweenHandshakeRetries)
{
    ArrangeHandshakeRetryThenSucceed(MBEDTLS_ERR_SSL_WANT_READ);

    SolidSyslogStream_Open(handle, addr);
    CALLED_FUNCTION(NoOpSleep, ONCE);
}

TEST(SolidSyslogMbedTlsStream, OpenRetriesHandshakeOnWantWrite)
{
    /* WANT_WRITE arises when mbedTLS needs to send (e.g. ClientFinished
     * under non-blocking transport with a temporarily-full send buffer).
     * Same retry treatment as WANT_READ. */
    ArrangeHandshakeRetryThenSucceed(MBEDTLS_ERR_SSL_WANT_WRITE);

    CHECK_TRUE(SolidSyslogStream_Open(handle, addr));
    CALLED_FAKE(MbedTlsFake_SslHandshake, TWICE);
}

TEST(SolidSyslogMbedTlsStream, OpenClosesTransportAndFreesSslStateWhenHandshakeBudgetExhausts)
{
    /* mbedtls_ssl_handshake always returns WANT_READ — handshake never makes
     * progress, so the bounded budget should expire and Open returns false. */
    ArrangePersistentHandshakeError(MBEDTLS_ERR_SSL_WANT_READ);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));
    CHECK_OPEN_UNWOUND_WITH_ERROR(transport, MBEDTLSSTREAM_ERROR_HANDSHAKE_TIMEOUT);
}

TEST(SolidSyslogMbedTlsStream, SecondOpenAfterFailedFirstOpenSucceeds)
{
    /* The recovery contract that the per-failure-point unwinds enable: once
     * Open's failure tail Closes the transport and frees the SSL state, the
     * next Open is a clean Open-Close-Open cycle on the transport — Connected
     * goes false, StreamSender's next reconnect tick re-enters, and the
     * second handshake completes. Without the unwind, the inner transport
     * would stay open and PosixTcpStream_Open would clobber its fd. */
    int handshakeSequence[] = {MBEDTLS_ERR_SSL_BAD_INPUT_DATA, 0};
    MbedTlsFake_SetSslHandshakeReturnSequence(handshakeSequence, 2);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));
    CHECK_TRUE(SolidSyslogStream_Open(handle, addr));
    LONGS_EQUAL(2, StreamFake_OpenCallCount(transport));
    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
}

TEST(SolidSyslogMbedTlsStream, OpenClosesTransportAndFreesSslStateWhenHandshakeFailsHard)
{
    /* Non-WANT error (e.g. a verify/connection failure) is fail-fast — no
     * retry budget burn, no Sleep. */
    ArrangePersistentHandshakeError(MBEDTLS_ERR_SSL_BAD_INPUT_DATA);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));
    CALLED_FAKE(MbedTlsFake_SslHandshake, ONCE);
    CALLED_FUNCTION(NoOpSleep, NEVER);
    CHECK_OPEN_UNWOUND_WITH_ERROR(transport, MBEDTLSSTREAM_ERROR_HANDSHAKE_REJECTED);
}

/* -------------------------------------------------------------------------
 * Open failure unwind + error reporting (S26.02). Every failure path after
 * the first allocating operation must close the inner transport, free both
 * mbedTLS structs, and emit the matching typed error code so the integrator
 * sees a protocol-level diagnostic.
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogMbedTlsStream, OpenClosesTransportAndFreesSslStateWhenSslConfigDefaultsFails)
{
    MbedTlsFake_SetSslConfigDefaultsReturn(-1);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));
    CHECK_OPEN_UNWOUND_WITH_ERROR(transport, MBEDTLSSTREAM_ERROR_DEFAULTS_NOT_APPLIED);
}

TEST(SolidSyslogMbedTlsStream, OpenClosesTransportAndFreesSslStateWhenSslSetupFails)
{
    MbedTlsFake_SetSslSetupReturn(-1);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));
    CHECK_OPEN_UNWOUND_WITH_ERROR(transport, MBEDTLSSTREAM_ERROR_SESSION_INIT_FAILED);
}

TEST(SolidSyslogMbedTlsStream, OpenClosesTransportAndFreesSslStateWhenSetHostnameFails)
{
    /* ServerName must be set for ConfigureExpectedHostname to invoke
     * mbedtls_ssl_set_hostname — otherwise the helper short-circuits to true. */
    config.ServerName = "syslog.example.com";
    ReCreateHandleWithUpdatedConfig();
    MbedTlsFake_SetSslSetHostnameReturn(-1);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));
    CHECK_OPEN_UNWOUND_WITH_ERROR(transport, MBEDTLSSTREAM_ERROR_SERVER_NAME_NOT_SET);
}

TEST(SolidSyslogMbedTlsStream, SendForwardsBufferToSslWrite)
{
    const unsigned char payload[] = {0x10, 0x20, 0x30};

    SolidSyslogStream_Send(handle, payload, sizeof(payload));

    LONGS_EQUAL(1, MbedTlsFake_SslWriteCallCount());
    POINTERS_EQUAL(payload, MbedTlsFake_LastSslWriteBufArg());
    LONGS_EQUAL(sizeof(payload), MbedTlsFake_LastSslWriteLenArg());
}

TEST(SolidSyslogMbedTlsStream, SendReturnsTrueWhenSslWriteWritesAllBytes)
{
    const unsigned char payload[] = {0x10, 0x20, 0x30};
    MbedTlsFake_SetSslWriteReturn((int) sizeof(payload));

    CHECK_TRUE(SolidSyslogStream_Send(handle, payload, sizeof(payload)));
}

TEST(SolidSyslogMbedTlsStream, SendReturnsFalseWhenSslWriteWritesPartial)
{
    const unsigned char payload[] = {0x10, 0x20, 0x30};
    MbedTlsFake_SetSslWriteReturn(1);

    CHECK_FALSE(SolidSyslogStream_Send(handle, payload, sizeof(payload)));
}

TEST(SolidSyslogMbedTlsStream, SendReturnsFalseWhenSslWriteFails)
{
    const unsigned char payload[] = {0x10, 0x20, 0x30};
    MbedTlsFake_SetSslWriteReturn(-1);

    CHECK_FALSE(SolidSyslogStream_Send(handle, payload, sizeof(payload)));
}

TEST(SolidSyslogMbedTlsStream, SendClosesSslAndTransportOnWriteFailure)
{
    /* Fail-fast: a TLS-level write failure means the session state is
     * unrecoverable. Mirror the OpenSSL TlsStream contract — close internally
     * so the StreamSender reconnect path runs on the next tick. */
    const unsigned char payload[] = {0x10, 0x20, 0x30};
    MbedTlsFake_SetSslWriteReturn(-1);

    SolidSyslogStream_Send(handle, payload, sizeof(payload));

    LONGS_EQUAL(1, MbedTlsFake_SslFreeCallCount());
    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
}

TEST(SolidSyslogMbedTlsStream, SendClosesSslAndTransportOnShortWrite)
{
    /* mbedtls_ssl_write returning fewer bytes than requested is treated the
     * same as outright failure — the application boundary requires
     * all-or-nothing writes (syslog framing). */
    const unsigned char payload[] = {0x10, 0x20, 0x30};
    MbedTlsFake_SetSslWriteReturn(2); /* asked for 3, got 2 */

    SolidSyslogStream_Send(handle, payload, sizeof(payload));

    LONGS_EQUAL(1, MbedTlsFake_SslFreeCallCount());
    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
}

TEST(SolidSyslogMbedTlsStream, ReadForwardsBufferToSslRead)
{
    unsigned char buffer[8];
    MbedTlsFake_SetSslReadReturn(3);

    SolidSyslogStream_Read(handle, buffer, sizeof(buffer));

    LONGS_EQUAL(1, MbedTlsFake_SslReadCallCount());
    POINTERS_EQUAL(buffer, MbedTlsFake_LastSslReadBufArg());
    LONGS_EQUAL(sizeof(buffer), MbedTlsFake_LastSslReadLenArg());
}

TEST(SolidSyslogMbedTlsStream, ReadReturnsByteCountWhenSslReadReturnsPositive)
{
    unsigned char buffer[8];
    MbedTlsFake_SetSslReadReturn(5);

    LONGS_EQUAL(5, SolidSyslogStream_Read(handle, buffer, sizeof(buffer)));
}

TEST(SolidSyslogMbedTlsStream, ReadReturnsZeroOnWantRead)
{
    unsigned char buffer[8];
    MbedTlsFake_SetSslReadReturn(MBEDTLS_ERR_SSL_WANT_READ);

    LONGS_EQUAL(0, SolidSyslogStream_Read(handle, buffer, sizeof(buffer)));
}

TEST(SolidSyslogMbedTlsStream, ReadReturnsNegativeOnSslReadError)
{
    unsigned char buffer[8];
    MbedTlsFake_SetSslReadReturn(-1);

    CHECK(SolidSyslogStream_Read(handle, buffer, sizeof(buffer)) < 0);
}

TEST(SolidSyslogMbedTlsStream, ReadClosesSslAndTransportOnHardError)
{
    /* Same fail-fast contract as Send: any read result other than positive
     * bytes or WANT_READ (e.g. peer close_notify, fatal alert, transport
     * error) closes internally so the next tick reopens. */
    unsigned char buffer[8];
    MbedTlsFake_SetSslReadReturn(-1);

    SolidSyslogStream_Read(handle, buffer, sizeof(buffer));

    LONGS_EQUAL(1, MbedTlsFake_SslFreeCallCount());
    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
}

TEST(SolidSyslogMbedTlsStream, ReadDoesNotCloseOnWantRead)
{
    /* WANT_READ is steady-state would-block, not a connection failure —
     * leave the session intact so the caller can retry. */
    unsigned char buffer[8];
    MbedTlsFake_SetSslReadReturn(MBEDTLS_ERR_SSL_WANT_READ);

    SolidSyslogStream_Read(handle, buffer, sizeof(buffer));

    LONGS_EQUAL(0, MbedTlsFake_SslFreeCallCount());
    LONGS_EQUAL(0, StreamFake_CloseCallCount(transport));
}

TEST(SolidSyslogMbedTlsStream, CloseAfterInternalCloseFromSendFailureDoesNotDoubleFree)
{
    /* Send and Read may close internally on failure; the subsequent Close
     * from the StreamSender reconnect path or Destroy must not crash or
     * double-free. mbedTLS's freed-equivalent zeroed state makes this safe. */
    const unsigned char payload[] = {0x10};
    MbedTlsFake_SetSslWriteReturn(-1);
    SolidSyslogStream_Send(handle, payload, sizeof(payload)); /* internal close */

    SolidSyslogStream_Close(handle); /* second close — must be safe */

    /* Exactly one of each free per real session. The teardown's Destroy
     * will add a third pair when this test ends, but that's outside the
     * assertion window. */
    LONGS_EQUAL(2, MbedTlsFake_SslFreeCallCount());
    LONGS_EQUAL(2, MbedTlsFake_SslConfigFreeCallCount());
}

TEST(SolidSyslogMbedTlsStream, CloseSendsSslCloseNotifyOnTheSslContextFromOpen)
{
    SolidSyslogStream_Open(handle, addr);

    SolidSyslogStream_Close(handle);

    LONGS_EQUAL(1, MbedTlsFake_SslCloseNotifyCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslInitArg(), MbedTlsFake_LastSslCloseNotifyArg());
}

TEST(SolidSyslogMbedTlsStream, CloseFreesSslContextAndSslConfigFromOpen)
{
    SolidSyslogStream_Open(handle, addr);

    SolidSyslogStream_Close(handle);

    LONGS_EQUAL(1, MbedTlsFake_SslFreeCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslInitArg(), MbedTlsFake_LastSslFreeArg());
    LONGS_EQUAL(1, MbedTlsFake_SslConfigFreeCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslConfigInitArg(), MbedTlsFake_LastSslConfigFreeArg());
}

TEST(SolidSyslogMbedTlsStream, CloseDelegatesToInjectedTransport)
{
    SolidSyslogStream_Close(handle);

    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
}

TEST(SolidSyslogMbedTlsStream, BioSendCallbackForwardsBufferToTransport)
{
    SolidSyslogStream_Open(handle, addr);
    auto* bioSend = MbedTlsFake_LastSslSetBioSendCallback();
    void* bioContext = MbedTlsFake_LastSslSetBioPBioArg();
    const unsigned char payload[] = {0xAA, 0xBB, 0xCC};

    int rc = bioSend(bioContext, payload, sizeof(payload));

    LONGS_EQUAL((int) sizeof(payload), rc);
    LONGS_EQUAL(1, StreamFake_SendCallCount(transport));
    POINTERS_EQUAL(payload, StreamFake_LastSendBuf(transport));
    LONGS_EQUAL(sizeof(payload), StreamFake_LastSendSize(transport));
}

TEST(SolidSyslogMbedTlsStream, BioRecvCallbackForwardsBufferToTransport)
{
    SolidSyslogStream_Open(handle, addr);
    auto* bioRecv = MbedTlsFake_LastSslSetBioRecvCallback();
    void* bioContext = MbedTlsFake_LastSslSetBioPBioArg();
    unsigned char buffer[16];
    StreamFake_SetReadReturn(transport, 4);

    int rc = bioRecv(bioContext, buffer, sizeof(buffer));

    LONGS_EQUAL(4, rc);
    LONGS_EQUAL(1, StreamFake_ReadCallCount(transport));
    POINTERS_EQUAL(buffer, StreamFake_LastReadBuf(transport));
    LONGS_EQUAL(sizeof(buffer), StreamFake_LastReadSize(transport));
}

TEST(SolidSyslogMbedTlsStream, BioRecvReturnsWantReadWhenTransportWouldBlock)
{
    /* Stream contract: transport Read returns 0 to signal would-block. mbedTLS
     * needs MBEDTLS_ERR_SSL_WANT_READ to drive its retry loop; any other
     * negative is fatal. Returning -1 (or 0) here would abort the handshake
     * on the first non-blocking poll. */
    SolidSyslogStream_Open(handle, addr);
    auto* bioRecv = MbedTlsFake_LastSslSetBioRecvCallback();
    void* bioContext = MbedTlsFake_LastSslSetBioPBioArg();
    unsigned char buffer[16];
    StreamFake_SetReadReturn(transport, 0);

    int rc = bioRecv(bioContext, buffer, sizeof(buffer));

    LONGS_EQUAL(MBEDTLS_ERR_SSL_WANT_READ, rc);
}

TEST(SolidSyslogMbedTlsStream, BioRecvReturnsFatalWhenTransportFails)
{
    /* Stream contract: negative is fatal. mbedTLS treats any negative other
     * than its own WANT_* sentinels as a transport error and aborts. */
    SolidSyslogStream_Open(handle, addr);
    auto* bioRecv = MbedTlsFake_LastSslSetBioRecvCallback();
    void* bioContext = MbedTlsFake_LastSslSetBioPBioArg();
    unsigned char buffer[16];
    StreamFake_SetReadReturn(transport, -1);

    int rc = bioRecv(bioContext, buffer, sizeof(buffer));

    CHECK_TRUE(rc < 0);
    CHECK_FALSE(rc == MBEDTLS_ERR_SSL_WANT_READ);
}

TEST(SolidSyslogMbedTlsStream, OpenSetsAuthmodeRequired)
{
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, MbedTlsFake_SslConfAuthmodeCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslConfigInitArg(), MbedTlsFake_LastSslConfAuthmodeConfigArg());
    LONGS_EQUAL(MBEDTLS_SSL_VERIFY_REQUIRED, MbedTlsFake_LastSslConfAuthmodeArg());
}

TEST(SolidSyslogMbedTlsStream, OpenWiresCaChainFromConfigAndNullCrl)
{
    /* Use a non-null marker pointer; the fake captures it without dereferencing. */
    static mbedtls_x509_crt caChainMarker;
    config.CaChain = &caChainMarker;
    ReCreateHandleWithUpdatedConfig();
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, MbedTlsFake_SslConfCaChainCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslConfigInitArg(), MbedTlsFake_LastSslConfCaChainConfigArg());
    POINTERS_EQUAL(&caChainMarker, MbedTlsFake_LastSslConfCaChainArg());
    POINTERS_EQUAL(nullptr, MbedTlsFake_LastSslConfCaChainCrlArg());
}

TEST(SolidSyslogMbedTlsStream, OpenWiresRngFromConfigUsingCtrDrbgRandom)
{
    static mbedtls_ctr_drbg_context rngMarker;
    config.Rng = &rngMarker;
    ReCreateHandleWithUpdatedConfig();
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, MbedTlsFake_SslConfRngCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslConfigInitArg(), MbedTlsFake_LastSslConfRngConfigArg());
    POINTERS_EQUAL((void*) mbedtls_ctr_drbg_random, (void*) MbedTlsFake_LastSslConfRngFuncArg());
    POINTERS_EQUAL(&rngMarker, MbedTlsFake_LastSslConfRngContextArg());
}

TEST(SolidSyslogMbedTlsStream, OpenSetsHostnameWhenServerNameProvided)
{
    config.ServerName = "syslog.example.com";
    ReCreateHandleWithUpdatedConfig();
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, MbedTlsFake_SslSetHostnameCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslInitArg(), MbedTlsFake_LastSslSetHostnameContextArg());
    STRCMP_EQUAL("syslog.example.com", MbedTlsFake_LastSslSetHostnameNameArg());
}

TEST(SolidSyslogMbedTlsStream, OpenSkipsHostnameWhenServerNameIsNull)
{
    /* setup() left config.ServerName at NULL. */
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(0, MbedTlsFake_SslSetHostnameCallCount());
}

/* -------------------------------------------------------------------------
 * mTLS client identity wiring. When the integrator supplies both a
 * ClientCertChain and a ClientKey, Open must call mbedtls_ssl_conf_own_cert
 * so the client presents its cert during the handshake. Either pointer
 * being NULL means "server-auth only" — skip the wiring.
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogMbedTlsStream, OpenWiresOwnCertWhenClientCertAndKeyProvided)
{
    static mbedtls_x509_crt clientCertMarker;
    static mbedtls_pk_context clientKeyMarker;
    config.ClientCertChain = &clientCertMarker;
    config.ClientKey = &clientKeyMarker;
    ReCreateHandleWithUpdatedConfig();
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, MbedTlsFake_SslConfOwnCertCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslConfigInitArg(), MbedTlsFake_LastSslConfOwnCertConfigArg());
    POINTERS_EQUAL(&clientCertMarker, MbedTlsFake_LastSslConfOwnCertCertArg());
    POINTERS_EQUAL(&clientKeyMarker, MbedTlsFake_LastSslConfOwnCertKeyArg());
}

TEST(SolidSyslogMbedTlsStream, OpenSkipsOwnCertWhenClientCertChainIsNull)
{
    /* Key provided, cert NULL — caller hasn't fully opted in to mTLS, so
     * the adapter must not tell mbedTLS anything. setup() leaves
     * ClientCertChain at NULL; supplying just a Key is the incomplete case. */
    static mbedtls_pk_context clientKeyMarker;
    config.ClientKey = &clientKeyMarker;
    ReCreateHandleWithUpdatedConfig();
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(0, MbedTlsFake_SslConfOwnCertCallCount());
}

TEST(SolidSyslogMbedTlsStream, OpenSkipsOwnCertWhenClientKeyIsNull)
{
    /* Cert provided, key NULL — still incomplete; same skip. */
    static mbedtls_x509_crt clientCertMarker;
    config.ClientCertChain = &clientCertMarker;
    ReCreateHandleWithUpdatedConfig();
    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(0, MbedTlsFake_SslConfOwnCertCallCount());
}
