#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/prov_ssl.h>
#include <openssl/types.h>
#include <stddef.h>

#include "OpenSslFake.h"
#include "AddressFake.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogTlsStream.h"
#include "SolidSyslogTransport.h"
#include "StreamFake.h"
#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros

class TEST_SolidSyslogTlsStream_ReadReturnsNegativeOneOnHardErrorAndClosesSsl_Test;
class TEST_SolidSyslogTlsStream_ReadReturnsNegativeOneOnZeroReturnAndClosesSsl_Test;
class TEST_SolidSyslogTlsStream_SendClosesTransportOnWriteFailure_Test;

static int NoOpSleepCallCount;
static int g_lastSleepMs;

static void NoOpSleep(int milliseconds)
{
    NoOpSleepCallCount++;
    g_lastSleepMs = milliseconds;
}

// clang-format off
TEST_GROUP(SolidSyslogTlsStream)
{
    struct SolidSyslogStream*         transport = nullptr;
    struct SolidSyslogTlsStreamConfig config    = {};
    struct SolidSyslogStream*         stream    = nullptr;
    struct SolidSyslogAddress*        addr      = nullptr;

    void setup() override
    {
        OpenSslFake_Reset();
        NoOpSleepCallCount = 0;
        g_lastSleepMs    = 0;
        transport        = StreamFake_Create();
        config.Transport = transport;
        config.Sleep     = NoOpSleep;
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        stream = SolidSyslogTlsStream_Create(&config);
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        addr = AddressFake_Get();
    }

    void teardown() override
    {
        SolidSyslogTlsStream_Destroy(stream);
        StreamFake_Destroy(transport);
    }

    /* Drive the registered BIO read callback with the given transport return —
       collapses the open + set-return + grab-callback + invoke boilerplate. */
    [[nodiscard]] int InvokeBioReadWithTransportReturn(SolidSyslogSsize transportReturn) const
    {
        SolidSyslogStream_Open(stream, addr);
        StreamFake_SetReadReturn(transport, transportReturn);
        int (*readFn)(BIO*, char*, int) = OpenSslFake_LastBioReadCallback();
        char buf[16];
        return readFn(OpenSslFake_LastBioReturned(), buf, sizeof(buf));
    }

    /* Drive the registered BIO write callback while the underlying transport
       Send is configured to fail. */
    [[nodiscard]] int InvokeBioWriteWithFailingTransport() const
    {
        SolidSyslogStream_Open(stream, addr);
        StreamFake_SetSendFails(transport, true);
        int (*writeFn)(BIO*, const char*, int) = OpenSslFake_LastBioWriteCallback();
        const char msg[]                       = "hi";
        return writeFn(OpenSslFake_LastBioReturned(), msg, (int) sizeof(msg));
    }

    /* Arrange SSL_connect to first emit `wantError`, then succeed on the next
       call — exercises the bounded handshake retry loop's progress path. */
    static void ArrangeHandshakeRetryThenSucceed(int wantError)
    {
        int seq[] = {-1, 1};
        OpenSslFake_SetConnectReturnSequence(seq, 2);
        OpenSslFake_SetGetErrorReturn(wantError);
    }

    /* Arrange SSL_connect to fail with `errorCode` on every call — used both
       for the persistent-WANT (budget-exhausted) and hard-error paths. */
    static void ArrangePersistentHandshakeError(int errorCode)
    {
        int seq[] = {-1};
        OpenSslFake_SetConnectReturnSequence(seq, 1);
        OpenSslFake_SetGetErrorReturn(errorCode);
    }

    /* Open then arrange the next SSL_write to fail — exercises the Send fail-fast
       teardown path that closes the SSL session and the underlying transport. */
    void OpenThenCauseSslWriteFailure() const
    {
        SolidSyslogStream_Open(stream, addr);
        OpenSslFake_SetWriteFails(true);
    }

    void SendShortMessage() const
    {
        const char msg[] = "hi";
        SolidSyslogStream_Send(stream, msg, sizeof(msg));
    }

    /* Open then arrange SSL_read to return the configured value while
       SSL_get_error reports the configured SSL-level status — together they
       exercise each branch of the Read non-blocking contract. */
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- both ints, but name + comment make role distinct
    [[nodiscard]] SolidSyslogSsize OpenThenReadWithSslReturnAndError(int sslReadReturn, int sslErrorCode) const
    {
        SolidSyslogStream_Open(stream, addr);
        OpenSslFake_SetReadReturn(sslReadReturn);
        OpenSslFake_SetGetErrorReturn(sslErrorCode);
        char buf[16];
        return SolidSyslogStream_Read(stream, buf, sizeof(buf));
    }
};

// clang-format on

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)
#define CHECK_BIO_READ_RETRY_SIGNALLED()            \
    do                                              \
    {                                               \
        CALLED_FAKE(OpenSslFake_BioSetFlags, ONCE); \
    } while (0)
#define CHECK_BIO_READ_RETRY_NOT_SIGNALLED()         \
    do                                               \
    {                                                \
        CALLED_FAKE(OpenSslFake_BioSetFlags, NEVER); \
    } while (0)
#define CHECK_BIO_RETRY_FLAGS_CLEARED()               \
    do                                                \
    {                                                 \
        CALLED_FAKE(OpenSslFake_BioClearFlags, ONCE); \
    } while (0)
#define CHECK_SSL_SESSION_CLOSED()               \
    do                                           \
    {                                            \
        CALLED_FAKE(OpenSslFake_Shutdown, ONCE); \
        CALLED_FAKE(OpenSslFake_Free, ONCE);     \
    } while (0)
#define CHECK_TRANSPORT_CLOSED_ONCE()                      \
    do                                                     \
    {                                                      \
        CALLED_FAKE_ON(StreamFake_Close, transport, ONCE); \
    } while (0)

// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

TEST(SolidSyslogTlsStream, CreateSucceeds)
{
    CHECK_TRUE(stream != nullptr);
}

TEST(SolidSyslogTlsStream, OpenOpensTransport)
{
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE_ON(StreamFake_Open, transport, ONCE);
}

TEST(SolidSyslogTlsStream, OpenPassesAddressToTransport)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(addr, StreamFake_LastOpenAddr(transport));
}

TEST(SolidSyslogTlsStream, OpenCreatesSslContext)
{
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_CtxNew, ONCE);
}

TEST(SolidSyslogTlsStream, OpenLoadsCaBundleFromConfig)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.CaBundlePath = "/some/path/ca.pem";
    stream = SolidSyslogTlsStream_Create(&config);
    SolidSyslogStream_Open(stream, addr);
    STRCMP_EQUAL("/some/path/ca.pem", OpenSslFake_LastCaBundlePath());
}

TEST(SolidSyslogTlsStream, OpenRequiresPeerVerification)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(SSL_VERIFY_PEER, OpenSslFake_LastVerifyMode());
}

TEST(SolidSyslogTlsStream, OpenSetsTls12Floor)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(TLS1_2_VERSION, OpenSslFake_LastMinProtoVersion());
}

TEST(SolidSyslogTlsStream, OpenPassesCipherListToSslCtx)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.CipherList = "ECDHE+AESGCM";
    stream = SolidSyslogTlsStream_Create(&config);
    SolidSyslogStream_Open(stream, addr);
    STRCMP_EQUAL("ECDHE+AESGCM", OpenSslFake_LastCipherList());
}

TEST(SolidSyslogTlsStream, OpenSkipsCipherListSetupWhenNotConfigured)
{
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_SetCipherList, NEVER);
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenCipherListRejected)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.CipherList = "not-a-real-cipher";
    stream = SolidSyslogTlsStream_Create(&config);
    OpenSslFake_SetCipherListFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, CipherListFailureFreesCtx)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.CipherList = "not-a-real-cipher";
    stream = SolidSyslogTlsStream_Create(&config);
    OpenSslFake_SetCipherListFails(true);
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_CtxFree, ONCE);
}

TEST(SolidSyslogTlsStream, OpenCreatesSslSession)
{
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_SslNew, ONCE);
}

TEST(SolidSyslogTlsStream, OpenPassesCtxFromCtxNewToSslNew)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastCtxReturned(), OpenSslFake_LastSslNewCtxArg());
}

TEST(SolidSyslogTlsStream, OpenCreatesBio)
{
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_BioNew, ONCE);
}

TEST(SolidSyslogTlsStream, OpenSetsBioOnSsl)
{
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_SetBio, ONCE);
}

TEST(SolidSyslogTlsStream, OpenPassesSslFromNewToSetBio)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastSslReturned(), OpenSslFake_LastSetBioSslArg());
}

TEST(SolidSyslogTlsStream, OpenPassesBioFromNewToSetBio)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastBioReturned(), OpenSslFake_LastSetBioReadBioArg());
}

TEST(SolidSyslogTlsStream, OpenPerformsHandshake)
{
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_Connect, ONCE);
}

TEST(SolidSyslogTlsStream, OpenPassesSslToConnect)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastSslReturned(), OpenSslFake_LastConnectSslArg());
}

TEST(SolidSyslogTlsStream, OpenSetsSniHostnameFromConfig)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ServerName = "logs.example";
    stream = SolidSyslogTlsStream_Create(&config);
    SolidSyslogStream_Open(stream, addr);
    STRCMP_EQUAL("logs.example", OpenSslFake_LastSniHostname());
}

TEST(SolidSyslogTlsStream, OpenSetsExpectedCertHostname)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ServerName = "logs.example";
    stream = SolidSyslogTlsStream_Create(&config);
    SolidSyslogStream_Open(stream, addr);
    STRCMP_EQUAL("logs.example", OpenSslFake_LastSet1Host());
}

TEST(SolidSyslogTlsStream, OpenSkipsHostnameSetupWhenServerNameIsNull)
{
    /* Default config.ServerName is NULL */
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(NULL, OpenSslFake_LastSet1Host());
}

TEST(SolidSyslogTlsStream, OpenAttachesTransportAsBioData)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(transport, OpenSslFake_LastSetDataArg());
}

TEST(SolidSyslogTlsStream, BioReadCallbackDelegatesToTransportRead)
{
    SolidSyslogStream_Open(stream, addr);
    int (*readFn)(BIO*, char*, int) = OpenSslFake_LastBioReadCallback();
    CHECK_TRUE(readFn != nullptr);
    if (readFn == nullptr)
    {
        return;
    }
    char buf[16];
    readFn(OpenSslFake_LastBioReturned(), buf, sizeof(buf));
    CALLED_FAKE_ON(StreamFake_Read, transport, ONCE);
}

TEST(SolidSyslogTlsStream, BioWriteCallbackDelegatesToTransportSend)
{
    SolidSyslogStream_Open(stream, addr);
    int (*writeFn)(BIO*, const char*, int) = OpenSslFake_LastBioWriteCallback();
    CHECK_TRUE(writeFn != nullptr);
    if (writeFn == nullptr)
    {
        return;
    }
    const char msg[] = "hi";
    writeFn(OpenSslFake_LastBioReturned(), msg, (int) sizeof(msg));
    CALLED_FAKE_ON(StreamFake_Send, transport, ONCE);
}

TEST(SolidSyslogTlsStream, SendWritesToSsl)
{
    SolidSyslogStream_Open(stream, addr);
    const char msg[] = "hello";
    SolidSyslogStream_Send(stream, msg, sizeof(msg));
    CALLED_FAKE(OpenSslFake_Write, ONCE);
}

TEST(SolidSyslogTlsStream, SendPassesBufferToSslWrite)
{
    SolidSyslogStream_Open(stream, addr);
    const char msg[] = "hello";
    SolidSyslogStream_Send(stream, msg, sizeof(msg));
    POINTERS_EQUAL(msg, OpenSslFake_LastWriteBuf());
}

TEST(SolidSyslogTlsStream, SendPassesSizeToSslWrite)
{
    SolidSyslogStream_Open(stream, addr);
    const char msg[] = "hello";
    SolidSyslogStream_Send(stream, msg, sizeof(msg));
    LONGS_EQUAL(sizeof(msg), OpenSslFake_LastWriteSize());
}

TEST(SolidSyslogTlsStream, SendPassesSslFromNewToWrite)
{
    SolidSyslogStream_Open(stream, addr);
    const char msg[] = "hello";
    SolidSyslogStream_Send(stream, msg, sizeof(msg));
    POINTERS_EQUAL(OpenSslFake_LastSslReturned(), OpenSslFake_LastWriteSslArg());
}

TEST(SolidSyslogTlsStream, ReadReadsFromSsl)
{
    SolidSyslogStream_Open(stream, addr);
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    CALLED_FAKE(OpenSslFake_SslRead, ONCE);
}

TEST(SolidSyslogTlsStream, ReadPassesSslFromNewToSslRead)
{
    SolidSyslogStream_Open(stream, addr);
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    POINTERS_EQUAL(OpenSslFake_LastSslReturned(), OpenSslFake_LastSslReadSslArg());
}

TEST(SolidSyslogTlsStream, ReadPassesBufferToSslRead)
{
    SolidSyslogStream_Open(stream, addr);
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    POINTERS_EQUAL(buf, OpenSslFake_LastSslReadBuf());
}

TEST(SolidSyslogTlsStream, ReadPassesSizeToSslRead)
{
    SolidSyslogStream_Open(stream, addr);
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    LONGS_EQUAL(sizeof(buf), OpenSslFake_LastSslReadSize());
}

TEST(SolidSyslogTlsStream, CloseShutsDownSsl)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    CALLED_FAKE(OpenSslFake_Shutdown, ONCE);
}

TEST(SolidSyslogTlsStream, CloseFreesSsl)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    CALLED_FAKE(OpenSslFake_Free, ONCE);
}

TEST(SolidSyslogTlsStream, CloseClosesTransport)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    CALLED_FAKE_ON(StreamFake_Close, transport, ONCE);
}

/* Destroy on a still-Open stream must release the underlying transport,
   otherwise an integrator that omits the explicit Close call leaks the
   transport's socket / fd. Verified before the teardown's Destroy fires
   (which would itself trigger another transport Close). */
TEST(SolidSyslogTlsStream, DestroyClosesTransportWhenStillOpen)
{
    SolidSyslogStream_Open(stream, addr);

    SolidSyslogTlsStream_Destroy(stream);

    CALLED_FAKE_ON(StreamFake_Close, transport, ONCE);
    /* Re-create so teardown's Destroy targets a live slot rather than a
       stale handle (which would fire SOLIDSYSLOG_ERROR_MSG_TLSSTREAM_UNKNOWN_DESTROY). */
    stream = SolidSyslogTlsStream_Create(&config);
}

TEST(SolidSyslogTlsStream, CloseFreesBioMethod)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    CALLED_FAKE(OpenSslFake_BioMethFree, ONCE);
}

TEST(SolidSyslogTlsStream, DestroyFreesSslContext)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogTlsStream_Destroy(stream);
    CALLED_FAKE(OpenSslFake_CtxFree, ONCE);
    /* teardown re-Destroys safely */
}

TEST(SolidSyslogTlsStream, DestroyFreesBioMethodWhenCloseNotCalled)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogTlsStream_Destroy(stream);
    CALLED_FAKE(OpenSslFake_BioMethFree, ONCE);
    /* teardown re-Destroys safely */
}

TEST(SolidSyslogTlsStream, DestroyAfterCloseDoesNotDoubleFreeBioMethod)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    SolidSyslogTlsStream_Destroy(stream);
    CALLED_FAKE(OpenSslFake_BioMethFree, ONCE);
}

TEST(SolidSyslogTlsStream, DestroyFreesSslWhenCloseNotCalled)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogTlsStream_Destroy(stream);
    CALLED_FAKE(OpenSslFake_Free, ONCE);
}

TEST(SolidSyslogTlsStream, DestroyAfterCloseDoesNotDoubleFreeSsl)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    SolidSyslogTlsStream_Destroy(stream);
    CALLED_FAKE(OpenSslFake_Free, ONCE);
}

/* -------------------------------------------------------------------------
 * Pointer-chain assertions: each OpenSSL call must receive the handle
 * returned by the preceding call, not some stale or NULL pointer.
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogTlsStream, OpenPassesClientMethodToCtxNew)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(TLS_client_method(), OpenSslFake_LastCtxNewMethodArg());
}

TEST(SolidSyslogTlsStream, OpenPassesCtxFromNewToLoadVerifyLocations)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastCtxReturned(), OpenSslFake_LastLoadVerifyLocationsCtxArg());
}

TEST(SolidSyslogTlsStream, OpenPassesCtxFromNewToSetVerify)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastCtxReturned(), OpenSslFake_LastSetVerifyCtxArg());
}

TEST(SolidSyslogTlsStream, OpenPassesCtxFromNewToSetMinProtoVersion)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastCtxReturned(), OpenSslFake_LastSslCtxCtrlCtxArg());
}

TEST(SolidSyslogTlsStream, OpenPassesBioMethodFromNewToSetRead)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastBioMethReturned(), OpenSslFake_LastBioMethSetReadMethodArg());
}

TEST(SolidSyslogTlsStream, OpenPassesBioMethodFromNewToSetWrite)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastBioMethReturned(), OpenSslFake_LastBioMethSetWriteMethodArg());
}

TEST(SolidSyslogTlsStream, OpenPassesBioMethodFromNewToBioNew)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastBioMethReturned(), OpenSslFake_LastBioNewMethodArg());
}

TEST(SolidSyslogTlsStream, OpenPassesBioFromNewToSetData)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastBioReturned(), OpenSslFake_LastSetDataBioArg());
}

TEST(SolidSyslogTlsStream, OpenPassesSameBioForReadAndWrite)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastSetBioReadBioArg(), OpenSslFake_LastSetBioWriteBioArg());
}

TEST(SolidSyslogTlsStream, OpenPassesSslToSniCtrl)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ServerName = "logs.example";
    stream = SolidSyslogTlsStream_Create(&config);
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastSslReturned(), OpenSslFake_LastSslCtrlSslArg());
}

TEST(SolidSyslogTlsStream, OpenPassesSslFromNewToSet1Host)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ServerName = "logs.example";
    stream = SolidSyslogTlsStream_Create(&config);
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastSslReturned(), OpenSslFake_LastSet1HostSslArg());
}

TEST(SolidSyslogTlsStream, BioReadCallbackLooksUpDataOnTheCorrectBio)
{
    SolidSyslogStream_Open(stream, addr);
    int (*readFn)(BIO*, char*, int) = OpenSslFake_LastBioReadCallback();
    char buf[16];
    readFn(OpenSslFake_LastBioReturned(), buf, sizeof(buf));
    POINTERS_EQUAL(OpenSslFake_LastBioReturned(), OpenSslFake_LastGetDataBioArg());
}

TEST(SolidSyslogTlsStream, ClosePassesSslFromNewToShutdown)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    POINTERS_EQUAL(OpenSslFake_LastSslReturned(), OpenSslFake_LastShutdownSslArg());
}

TEST(SolidSyslogTlsStream, ClosePassesSslFromNewToFree)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    POINTERS_EQUAL(OpenSslFake_LastSslReturned(), OpenSslFake_LastFreeSslArg());
}

TEST(SolidSyslogTlsStream, DestroyPassesCtxFromNewToCtxFree)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogTlsStream_Destroy(stream);
    POINTERS_EQUAL(OpenSslFake_LastCtxReturned(), OpenSslFake_LastCtxFreeCtxArg());
}

/* -------------------------------------------------------------------------
 * Error paths.
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogTlsStream, OpenReturnsTrueOnHappyPath)
{
    CHECK_TRUE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenHandshakeFails)
{
    OpenSslFake_SetConnectFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenSet1HostFails)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ServerName = "logs.example";
    stream = SolidSyslogTlsStream_Create(&config);
    OpenSslFake_SetSet1HostFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenSniHostnameSetupFails)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ServerName = "logs.example";
    stream = SolidSyslogTlsStream_Create(&config);
    OpenSslFake_SetSniHostnameFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenCtxNewFails)
{
    OpenSslFake_SetCtxNewFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenSslNewFails)
{
    OpenSslFake_SetSslNewFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenLoadVerifyLocationsFails)
{
    OpenSslFake_SetLoadVerifyLocationsFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, LoadVerifyLocationsFailureFreesCtx)
{
    OpenSslFake_SetLoadVerifyLocationsFails(true);
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_CtxFree, ONCE);
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenMinProtoVersionFails)
{
    OpenSslFake_SetMinProtoVersionFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, MinProtoVersionFailureFreesCtx)
{
    OpenSslFake_SetMinProtoVersionFails(true);
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_CtxFree, ONCE);
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenBioMethNewFails)
{
    OpenSslFake_SetBioMethNewFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenBioNewFails)
{
    OpenSslFake_SetBioNewFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, BioNewFailureFreesBioMethodInline)
{
    OpenSslFake_SetBioNewFails(true);
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_BioMethFree, ONCE);
    /* teardown re-Destroys safely — bioMethod already cleared */
}

TEST(SolidSyslogTlsStream, SendReturnsTrueOnHappyPath)
{
    SolidSyslogStream_Open(stream, addr);
    const char msg[] = "hi";
    CHECK_TRUE(SolidSyslogStream_Send(stream, msg, sizeof(msg)));
}

TEST(SolidSyslogTlsStream, SendReturnsFalseWhenWriteFails)
{
    SolidSyslogStream_Open(stream, addr);
    OpenSslFake_SetWriteFails(true);
    const char msg[] = "hi";
    CHECK_FALSE(SolidSyslogStream_Send(stream, msg, sizeof(msg)));
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenTransportOpenFails)
{
    StreamFake_SetOpenFails(transport, true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, OpenSkipsSslSetupWhenTransportOpenFails)
{
    StreamFake_SetOpenFails(transport, true);
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_CtxNew, NEVER);
}

TEST(SolidSyslogTlsStream, OpenWiresBioCtrlCallback)
{
    SolidSyslogStream_Open(stream, addr);
    CHECK_TRUE(OpenSslFake_LastBioCtrlCallback() != nullptr);
}

TEST(SolidSyslogTlsStream, OpenWiresBioCreateCallback)
{
    SolidSyslogStream_Open(stream, addr);
    CHECK_TRUE(OpenSslFake_LastBioCreateCallback() != nullptr);
}

TEST(SolidSyslogTlsStream, BioCtrlCallbackReturnsSuccessForFlush)
{
    SolidSyslogStream_Open(stream, addr);
    auto ctrlFn = OpenSslFake_LastBioCtrlCallback();
    LONGS_EQUAL(1, ctrlFn(OpenSslFake_LastBioReturned(), BIO_CTRL_FLUSH, 0, nullptr));
}

TEST(SolidSyslogTlsStream, BioCtrlCallbackReturnsSuccessForPushPopDup)
{
    SolidSyslogStream_Open(stream, addr);
    auto ctrlFn = OpenSslFake_LastBioCtrlCallback();
    BIO* bio = OpenSslFake_LastBioReturned();
    LONGS_EQUAL(1, ctrlFn(bio, BIO_CTRL_PUSH, 0, nullptr));
    LONGS_EQUAL(1, ctrlFn(bio, BIO_CTRL_POP, 0, nullptr));
    LONGS_EQUAL(1, ctrlFn(bio, BIO_CTRL_DUP, 0, nullptr));
}

TEST(SolidSyslogTlsStream, BioCtrlCallbackReturnsFailureForUnknownCommand)
{
    SolidSyslogStream_Open(stream, addr);
    auto ctrlFn = OpenSslFake_LastBioCtrlCallback();
    LONGS_EQUAL(0, ctrlFn(OpenSslFake_LastBioReturned(), /* arbitrary unsupported cmd */ 9999, 0, nullptr));
}

TEST(SolidSyslogTlsStream, BioCreateCallbackMarksBioInitialised)
{
    SolidSyslogStream_Open(stream, addr);
    auto createFn = OpenSslFake_LastBioCreateCallback();
    createFn(OpenSslFake_LastBioReturned());
    LONGS_EQUAL(1, OpenSslFake_LastSetInitArg());
}

/* -------------------------------------------------------------------------
 * Mutual TLS — client certificate + private key (S03.09).
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogTlsStream, OpenSkipsClientIdentityWhenBothPathsAreNull)
{
    /* Default config: clientCertChainPath and clientKeyPath both NULL. */
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_UseCertChainFile, NEVER);
    CALLED_FAKE(OpenSslFake_UsePrivateKeyFile, NEVER);
    CALLED_FAKE(OpenSslFake_CheckPrivateKey, NEVER);
}

TEST(SolidSyslogTlsStream, OpenLoadsClientCertChainFromConfig)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ClientCertChainPath = "/some/path/client.pem";
    config.ClientKeyPath = "/some/path/client.key";
    stream = SolidSyslogTlsStream_Create(&config);
    SolidSyslogStream_Open(stream, addr);
    STRCMP_EQUAL("/some/path/client.pem", OpenSslFake_LastClientCertChainPath());
}

TEST(SolidSyslogTlsStream, OpenLoadsClientKeyFromConfig)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ClientCertChainPath = "/some/path/client.pem";
    config.ClientKeyPath = "/some/path/client.key";
    stream = SolidSyslogTlsStream_Create(&config);
    SolidSyslogStream_Open(stream, addr);
    STRCMP_EQUAL("/some/path/client.key", OpenSslFake_LastClientKeyPath());
    LONGS_EQUAL(SSL_FILETYPE_PEM, OpenSslFake_LastClientKeyFileType());
}

TEST(SolidSyslogTlsStream, OpenChecksClientKeyMatchesCert)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ClientCertChainPath = "/some/path/client.pem";
    config.ClientKeyPath = "/some/path/client.key";
    stream = SolidSyslogTlsStream_Create(&config);
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_CheckPrivateKey, ONCE);
}

TEST(SolidSyslogTlsStream, OpenFailsWhenOnlyClientCertIsSet)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ClientCertChainPath = "/some/path/client.pem";
    config.ClientKeyPath = nullptr;
    stream = SolidSyslogTlsStream_Create(&config);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, OpenMakesNoClientIdentityCallsWhenOnlyClientCertIsSet)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ClientCertChainPath = "/some/path/client.pem";
    config.ClientKeyPath = nullptr;
    stream = SolidSyslogTlsStream_Create(&config);
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_UseCertChainFile, NEVER);
    CALLED_FAKE(OpenSslFake_UsePrivateKeyFile, NEVER);
    CALLED_FAKE(OpenSslFake_CheckPrivateKey, NEVER);
}

TEST(SolidSyslogTlsStream, OpenFailsWhenOnlyClientKeyIsSet)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ClientCertChainPath = nullptr;
    config.ClientKeyPath = "/some/path/client.key";
    stream = SolidSyslogTlsStream_Create(&config);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, OpenMakesNoClientIdentityCallsWhenOnlyClientKeyIsSet)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ClientCertChainPath = nullptr;
    config.ClientKeyPath = "/some/path/client.key";
    stream = SolidSyslogTlsStream_Create(&config);
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_UseCertChainFile, NEVER);
    CALLED_FAKE(OpenSslFake_UsePrivateKeyFile, NEVER);
    CALLED_FAKE(OpenSslFake_CheckPrivateKey, NEVER);
}

TEST(SolidSyslogTlsStream, PartialClientIdentityConfigFreesCtx)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ClientCertChainPath = "/some/path/client.pem";
    config.ClientKeyPath = nullptr;
    stream = SolidSyslogTlsStream_Create(&config);
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_CtxFree, ONCE);
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenUseCertChainFileFails)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ClientCertChainPath = "/some/path/client.pem";
    config.ClientKeyPath = "/some/path/client.key";
    stream = SolidSyslogTlsStream_Create(&config);
    OpenSslFake_SetUseCertChainFileFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, UseCertChainFileFailureFreesCtx)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ClientCertChainPath = "/some/path/client.pem";
    config.ClientKeyPath = "/some/path/client.key";
    stream = SolidSyslogTlsStream_Create(&config);
    OpenSslFake_SetUseCertChainFileFails(true);
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_CtxFree, ONCE);
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenUsePrivateKeyFileFails)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ClientCertChainPath = "/some/path/client.pem";
    config.ClientKeyPath = "/some/path/client.key";
    stream = SolidSyslogTlsStream_Create(&config);
    OpenSslFake_SetUsePrivateKeyFileFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, UsePrivateKeyFileFailureFreesCtx)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ClientCertChainPath = "/some/path/client.pem";
    config.ClientKeyPath = "/some/path/client.key";
    stream = SolidSyslogTlsStream_Create(&config);
    OpenSslFake_SetUsePrivateKeyFileFails(true);
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_CtxFree, ONCE);
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenCheckPrivateKeyFails)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ClientCertChainPath = "/some/path/client.pem";
    config.ClientKeyPath = "/some/path/client.key";
    stream = SolidSyslogTlsStream_Create(&config);
    OpenSslFake_SetCheckPrivateKeyFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, CheckPrivateKeyFailureFreesCtx)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ClientCertChainPath = "/some/path/client.pem";
    config.ClientKeyPath = "/some/path/client.key";
    stream = SolidSyslogTlsStream_Create(&config);
    OpenSslFake_SetCheckPrivateKeyFails(true);
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(OpenSslFake_CtxFree, ONCE);
}

TEST(SolidSyslogTlsStream, OpenPassesCtxFromNewToUseCertChainFile)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ClientCertChainPath = "/some/path/client.pem";
    config.ClientKeyPath = "/some/path/client.key";
    stream = SolidSyslogTlsStream_Create(&config);
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastCtxReturned(), OpenSslFake_LastUseCertChainFileCtxArg());
}

TEST(SolidSyslogTlsStream, OpenPassesCtxFromNewToUsePrivateKeyFile)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ClientCertChainPath = "/some/path/client.pem";
    config.ClientKeyPath = "/some/path/client.key";
    stream = SolidSyslogTlsStream_Create(&config);
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastCtxReturned(), OpenSslFake_LastUsePrivateKeyFileCtxArg());
}

TEST(SolidSyslogTlsStream, OpenPassesCtxFromNewToCheckPrivateKey)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.ClientCertChainPath = "/some/path/client.pem";
    config.ClientKeyPath = "/some/path/client.key";
    stream = SolidSyslogTlsStream_Create(&config);
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastCtxReturned(), OpenSslFake_LastCheckPrivateKeyCtxArg());
}

TEST(SolidSyslogTlsStream, DefaultPortMatchesRfc5425)
{
    LONGS_EQUAL(6514, SOLIDSYSLOG_TLS_DEFAULT_PORT);
}

/* -------------------------------------------------------------------------
 * Non-blocking BIO read translation. Under the new transport contract a
 * 0 return means "would-block, retry"; without BIO_set_retry_read OpenSSL
 * would treat that as EOF and abort the handshake on the first poll.
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogTlsStream, BioReadCallbackSignalsRetryWhenTransportWouldBlock)
{
    LONGS_EQUAL(-1, InvokeBioReadWithTransportReturn(0));
    CHECK_BIO_READ_RETRY_SIGNALLED();
}

TEST(SolidSyslogTlsStream, BioReadCallbackClearsRetryOnHardError)
{
    LONGS_EQUAL(-1, InvokeBioReadWithTransportReturn(-1));
    CHECK_BIO_RETRY_FLAGS_CLEARED();
}

TEST(SolidSyslogTlsStream, BioReadCallbackReturnsBytesWhenTransportHasData)
{
    LONGS_EQUAL(7, InvokeBioReadWithTransportReturn(7));
    /* No retry signal needed: positive return is the success path. */
    CHECK_BIO_READ_RETRY_NOT_SIGNALLED();
}

TEST(SolidSyslogTlsStream, BioWriteCallbackClearsRetryOnTransportFailure)
{
    /* When the transport's fail-fast Send returns false the BIO must clear
       any stale retry flag and return -1 so OpenSSL surfaces SSL_ERROR_SYSCALL
       rather than spinning on a closed transport. */
    LONGS_EQUAL(-1, InvokeBioWriteWithFailingTransport());
    CHECK_BIO_RETRY_FLAGS_CLEARED();
}

/* -------------------------------------------------------------------------
 * Bounded handshake retry loop. SSL_connect under non-blocking transport
 * will emit WANT_READ/WANT_WRITE between RTTs; the loop must drive it to
 * completion within HANDSHAKE_TIMEOUT_MILLISECONDS.
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogTlsStream, OpenRetriesHandshakeOnWantRead)
{
    ArrangeHandshakeRetryThenSucceed(SSL_ERROR_WANT_READ);
    CHECK_TRUE(SolidSyslogStream_Open(stream, addr));
    CALLED_FAKE(OpenSslFake_Connect, TWICE);
}

TEST(SolidSyslogTlsStream, OpenSleepsBetweenHandshakeRetries)
{
    ArrangeHandshakeRetryThenSucceed(SSL_ERROR_WANT_READ);
    SolidSyslogStream_Open(stream, addr);
    CALLED_FUNCTION(NoOpSleep, ONCE);
}

TEST(SolidSyslogTlsStream, OpenRetriesHandshakeOnWantWrite)
{
    /* WANT_WRITE arises when SSL needs to send (e.g. during the handshake
       finished message under non-blocking transport with a temporarily-full
       send buffer). Same retry treatment as WANT_READ. */
    ArrangeHandshakeRetryThenSucceed(SSL_ERROR_WANT_WRITE);
    CHECK_TRUE(SolidSyslogStream_Open(stream, addr));
    CALLED_FAKE(OpenSslFake_Connect, TWICE);
}

TEST(SolidSyslogTlsStream, OpenFailsWhenHandshakeNeverCompletes)
{
    /* SSL_connect always returns -1 with WANT_READ — handshake never makes
       progress, so the bounded budget should expire and Open returns false. */
    ArrangePersistentHandshakeError(SSL_ERROR_WANT_READ);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, OpenFailsImmediatelyOnHardSslError)
{
    /* Non-WANT error (e.g. SSL_ERROR_SSL) is fail-fast — no retry budget burn. */
    ArrangePersistentHandshakeError(SSL_ERROR_SSL);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
    CALLED_FAKE(OpenSslFake_Connect, ONCE);
    CALLED_FUNCTION(NoOpSleep, NEVER);
}

/* -------------------------------------------------------------------------
 * Send fail-fast: any non-success closes the SSL session and the underlying
 * transport so the StreamSender's reconnect path runs on the next tick.
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogTlsStream, SendClosesSslOnWriteFailure)
{
    OpenThenCauseSslWriteFailure();
    SendShortMessage();
    CHECK_SSL_SESSION_CLOSED();
}

TEST(SolidSyslogTlsStream, SendClosesTransportOnWriteFailure)
{
    OpenThenCauseSslWriteFailure();
    SendShortMessage();
    CHECK_TRANSPORT_CLOSED_ONCE();
}

TEST(SolidSyslogTlsStream, SendReturnsFalseOnShortWrite)
{
    SolidSyslogStream_Open(stream, addr);
    OpenSslFake_SetWriteReturn(3);
    const char msg[] = "hello";
    CHECK_FALSE(SolidSyslogStream_Send(stream, msg, sizeof(msg)));
}

/* -------------------------------------------------------------------------
 * Read non-blocking contract.
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogTlsStream, ReadReturnsZeroOnWantRead)
{
    LONGS_EQUAL(0, OpenThenReadWithSslReturnAndError(-1, SSL_ERROR_WANT_READ));
}

TEST(SolidSyslogTlsStream, ReadReturnsNegativeOneOnHardErrorAndClosesSsl)
{
    LONGS_EQUAL(-1, OpenThenReadWithSslReturnAndError(-1, SSL_ERROR_SSL));
    CHECK_SSL_SESSION_CLOSED();
    CHECK_TRANSPORT_CLOSED_ONCE();
}

TEST(SolidSyslogTlsStream, ReadReturnsNegativeOneOnZeroReturnAndClosesSsl)
{
    /* SSL_read returns 0 → SSL_ERROR_ZERO_RETURN (clean shutdown by peer). */
    LONGS_EQUAL(-1, OpenThenReadWithSslReturnAndError(0, SSL_ERROR_ZERO_RETURN));
    CHECK_SSL_SESSION_CLOSED();
    CHECK_TRANSPORT_CLOSED_ONCE();
}

/* -------------------------------------------------------------------------
 * Close idempotency. Send/Read may close internally on failure; subsequent
 * Close from the StreamSender's reconnect path or Destroy must not crash
 * or double-free.
 * ------------------------------------------------------------------------- */

TEST(SolidSyslogTlsStream, CloseAfterInternalCloseFromSendFailureDoesNotDoubleFree)
{
    OpenThenCauseSslWriteFailure();
    SendShortMessage(); /* internal close */
    SolidSyslogStream_Close(stream); /* second close — must be safe */
    CHECK_SSL_SESSION_CLOSED();
}
