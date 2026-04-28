#include "CppUTest/TestHarness.h"
#include "OpenSslFake.h"
#include "SolidSyslogAddress.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogTlsStream.h"
#include "SolidSyslogTransport.h"
#include "StreamFake.h"
#include <openssl/ssl.h>

// clang-format off
TEST_GROUP(SolidSyslogTlsStream)
{
    struct SolidSyslogStream*         transport   = nullptr;
    struct SolidSyslogTlsStreamConfig config      = {};
    SolidSyslogTlsStreamStorage       streamStorage{};
    struct SolidSyslogStream*         stream      = nullptr;
    SolidSyslogAddressStorage         addrStorage = {0};
    struct SolidSyslogAddress*        addr        = nullptr;

    void setup() override
    {
        OpenSslFake_Reset();
        transport        = StreamFake_Create();
        config.transport = transport;
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        stream = SolidSyslogTlsStream_Create(&streamStorage, &config);
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        addr = SolidSyslogAddress_FromStorage(&addrStorage);
    }

    void teardown() override
    {
        SolidSyslogTlsStream_Destroy(stream);
        StreamFake_Destroy(transport);
    }
};

// clang-format on

TEST(SolidSyslogTlsStream, CreateSucceeds)
{
    CHECK_TRUE(stream != nullptr);
}

TEST(SolidSyslogTlsStream, CreateReturnsHandleInsideCallerSuppliedStorage)
{
    SolidSyslogTlsStreamStorage       localStorage{};
    struct SolidSyslogTlsStreamConfig localConfig = {};
    localConfig.transport                         = transport;
    struct SolidSyslogStream* localStream         = SolidSyslogTlsStream_Create(&localStorage, &localConfig);
    POINTERS_EQUAL(&localStorage, localStream);
    SolidSyslogTlsStream_Destroy(localStream);
}

TEST(SolidSyslogTlsStream, OpenOpensTransport)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, StreamFake_OpenCallCount(transport));
}

TEST(SolidSyslogTlsStream, OpenPassesAddressToTransport)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(addr, StreamFake_LastOpenAddr(transport));
}

TEST(SolidSyslogTlsStream, OpenCreatesSslContext)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, OpenSslFake_CtxNewCallCount());
}

TEST(SolidSyslogTlsStream, OpenLoadsCaBundleFromConfig)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.caBundlePath = "/some/path/ca.pem";
    stream              = SolidSyslogTlsStream_Create(&streamStorage, &config);
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
    config.cipherList = "ECDHE+AESGCM";
    stream            = SolidSyslogTlsStream_Create(&streamStorage, &config);
    SolidSyslogStream_Open(stream, addr);
    STRCMP_EQUAL("ECDHE+AESGCM", OpenSslFake_LastCipherList());
}

TEST(SolidSyslogTlsStream, OpenSkipsCipherListSetupWhenNotConfigured)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(0, OpenSslFake_SetCipherListCallCount());
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenCipherListRejected)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.cipherList = "not-a-real-cipher";
    stream            = SolidSyslogTlsStream_Create(&streamStorage, &config);
    OpenSslFake_SetCipherListFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, CipherListFailureFreesCtx)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.cipherList = "not-a-real-cipher";
    stream            = SolidSyslogTlsStream_Create(&streamStorage, &config);
    OpenSslFake_SetCipherListFails(true);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, OpenSslFake_CtxFreeCallCount());
}

TEST(SolidSyslogTlsStream, OpenCreatesSslSession)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, OpenSslFake_SslNewCallCount());
}

TEST(SolidSyslogTlsStream, OpenPassesCtxFromCtxNewToSslNew)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastCtxReturned(), OpenSslFake_LastSslNewCtxArg());
}

TEST(SolidSyslogTlsStream, OpenCreatesBio)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, OpenSslFake_BioNewCallCount());
}

TEST(SolidSyslogTlsStream, OpenSetsBioOnSsl)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, OpenSslFake_SetBioCallCount());
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
    LONGS_EQUAL(1, OpenSslFake_ConnectCallCount());
}

TEST(SolidSyslogTlsStream, OpenPassesSslToConnect)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastSslReturned(), OpenSslFake_LastConnectSslArg());
}

TEST(SolidSyslogTlsStream, OpenSetsSniHostnameFromConfig)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.serverName = "logs.example";
    stream            = SolidSyslogTlsStream_Create(&streamStorage, &config);
    SolidSyslogStream_Open(stream, addr);
    STRCMP_EQUAL("logs.example", OpenSslFake_LastSniHostname());
}

TEST(SolidSyslogTlsStream, OpenSetsExpectedCertHostname)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.serverName = "logs.example";
    stream            = SolidSyslogTlsStream_Create(&streamStorage, &config);
    SolidSyslogStream_Open(stream, addr);
    STRCMP_EQUAL("logs.example", OpenSslFake_LastSet1Host());
}

TEST(SolidSyslogTlsStream, OpenSkipsHostnameSetupWhenServerNameIsNull)
{
    /* Default config.serverName is NULL */
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
    LONGS_EQUAL(1, StreamFake_ReadCallCount(transport));
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
    LONGS_EQUAL(1, StreamFake_SendCallCount(transport));
}

TEST(SolidSyslogTlsStream, SendWritesToSsl)
{
    SolidSyslogStream_Open(stream, addr);
    const char msg[] = "hello";
    SolidSyslogStream_Send(stream, msg, sizeof(msg));
    LONGS_EQUAL(1, OpenSslFake_WriteCallCount());
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
    LONGS_EQUAL(1, OpenSslFake_SslReadCallCount());
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
    LONGS_EQUAL(1, OpenSslFake_ShutdownCallCount());
}

TEST(SolidSyslogTlsStream, CloseFreesSsl)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    LONGS_EQUAL(1, OpenSslFake_FreeCallCount());
}

TEST(SolidSyslogTlsStream, CloseClosesTransport)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
}

TEST(SolidSyslogTlsStream, CloseFreesBioMethod)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    LONGS_EQUAL(1, OpenSslFake_BioMethFreeCallCount());
}

TEST(SolidSyslogTlsStream, DestroyFreesSslContext)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogTlsStream_Destroy(stream);
    LONGS_EQUAL(1, OpenSslFake_CtxFreeCallCount());
    /* teardown re-Destroys safely */
}

TEST(SolidSyslogTlsStream, DestroyFreesBioMethodWhenCloseNotCalled)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogTlsStream_Destroy(stream);
    LONGS_EQUAL(1, OpenSslFake_BioMethFreeCallCount());
    /* teardown re-Destroys safely */
}

TEST(SolidSyslogTlsStream, DestroyAfterCloseDoesNotDoubleFreeBioMethod)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    SolidSyslogTlsStream_Destroy(stream);
    LONGS_EQUAL(1, OpenSslFake_BioMethFreeCallCount());
}

TEST(SolidSyslogTlsStream, DestroyFreesSslWhenCloseNotCalled)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogTlsStream_Destroy(stream);
    LONGS_EQUAL(1, OpenSslFake_FreeCallCount());
}

TEST(SolidSyslogTlsStream, DestroyAfterCloseDoesNotDoubleFreeSsl)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    SolidSyslogTlsStream_Destroy(stream);
    LONGS_EQUAL(1, OpenSslFake_FreeCallCount());
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
    config.serverName = "logs.example";
    stream            = SolidSyslogTlsStream_Create(&streamStorage, &config);
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastSslReturned(), OpenSslFake_LastSslCtrlSslArg());
}

TEST(SolidSyslogTlsStream, OpenPassesSslFromNewToSet1Host)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.serverName = "logs.example";
    stream            = SolidSyslogTlsStream_Create(&streamStorage, &config);
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
    config.serverName = "logs.example";
    stream            = SolidSyslogTlsStream_Create(&streamStorage, &config);
    OpenSslFake_SetSet1HostFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenSniHostnameSetupFails)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.serverName = "logs.example";
    stream            = SolidSyslogTlsStream_Create(&streamStorage, &config);
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
    LONGS_EQUAL(1, OpenSslFake_CtxFreeCallCount());
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
    LONGS_EQUAL(1, OpenSslFake_CtxFreeCallCount());
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
    LONGS_EQUAL(1, OpenSslFake_BioMethFreeCallCount());
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
    LONGS_EQUAL(0, OpenSslFake_CtxNewCallCount());
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
    BIO* bio    = OpenSslFake_LastBioReturned();
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
    LONGS_EQUAL(0, OpenSslFake_UseCertChainFileCallCount());
    LONGS_EQUAL(0, OpenSslFake_UsePrivateKeyFileCallCount());
    LONGS_EQUAL(0, OpenSslFake_CheckPrivateKeyCallCount());
}

TEST(SolidSyslogTlsStream, OpenLoadsClientCertChainFromConfig)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.clientCertChainPath = "/some/path/client.pem";
    config.clientKeyPath       = "/some/path/client.key";
    stream                     = SolidSyslogTlsStream_Create(&streamStorage, &config);
    SolidSyslogStream_Open(stream, addr);
    STRCMP_EQUAL("/some/path/client.pem", OpenSslFake_LastClientCertChainPath());
}

TEST(SolidSyslogTlsStream, OpenLoadsClientKeyFromConfig)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.clientCertChainPath = "/some/path/client.pem";
    config.clientKeyPath       = "/some/path/client.key";
    stream                     = SolidSyslogTlsStream_Create(&streamStorage, &config);
    SolidSyslogStream_Open(stream, addr);
    STRCMP_EQUAL("/some/path/client.key", OpenSslFake_LastClientKeyPath());
    LONGS_EQUAL(SSL_FILETYPE_PEM, OpenSslFake_LastClientKeyFileType());
}

TEST(SolidSyslogTlsStream, OpenChecksClientKeyMatchesCert)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.clientCertChainPath = "/some/path/client.pem";
    config.clientKeyPath       = "/some/path/client.key";
    stream                     = SolidSyslogTlsStream_Create(&streamStorage, &config);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, OpenSslFake_CheckPrivateKeyCallCount());
}

TEST(SolidSyslogTlsStream, OpenFailsWhenOnlyClientCertIsSet)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.clientCertChainPath = "/some/path/client.pem";
    config.clientKeyPath       = nullptr;
    stream                     = SolidSyslogTlsStream_Create(&streamStorage, &config);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, OpenMakesNoClientIdentityCallsWhenOnlyClientCertIsSet)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.clientCertChainPath = "/some/path/client.pem";
    config.clientKeyPath       = nullptr;
    stream                     = SolidSyslogTlsStream_Create(&streamStorage, &config);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(0, OpenSslFake_UseCertChainFileCallCount());
    LONGS_EQUAL(0, OpenSslFake_UsePrivateKeyFileCallCount());
    LONGS_EQUAL(0, OpenSslFake_CheckPrivateKeyCallCount());
}

TEST(SolidSyslogTlsStream, OpenFailsWhenOnlyClientKeyIsSet)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.clientCertChainPath = nullptr;
    config.clientKeyPath       = "/some/path/client.key";
    stream                     = SolidSyslogTlsStream_Create(&streamStorage, &config);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, OpenMakesNoClientIdentityCallsWhenOnlyClientKeyIsSet)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.clientCertChainPath = nullptr;
    config.clientKeyPath       = "/some/path/client.key";
    stream                     = SolidSyslogTlsStream_Create(&streamStorage, &config);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(0, OpenSslFake_UseCertChainFileCallCount());
    LONGS_EQUAL(0, OpenSslFake_UsePrivateKeyFileCallCount());
    LONGS_EQUAL(0, OpenSslFake_CheckPrivateKeyCallCount());
}

TEST(SolidSyslogTlsStream, PartialClientIdentityConfigFreesCtx)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.clientCertChainPath = "/some/path/client.pem";
    config.clientKeyPath       = nullptr;
    stream                     = SolidSyslogTlsStream_Create(&streamStorage, &config);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, OpenSslFake_CtxFreeCallCount());
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenUseCertChainFileFails)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.clientCertChainPath = "/some/path/client.pem";
    config.clientKeyPath       = "/some/path/client.key";
    stream                     = SolidSyslogTlsStream_Create(&streamStorage, &config);
    OpenSslFake_SetUseCertChainFileFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, UseCertChainFileFailureFreesCtx)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.clientCertChainPath = "/some/path/client.pem";
    config.clientKeyPath       = "/some/path/client.key";
    stream                     = SolidSyslogTlsStream_Create(&streamStorage, &config);
    OpenSslFake_SetUseCertChainFileFails(true);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, OpenSslFake_CtxFreeCallCount());
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenUsePrivateKeyFileFails)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.clientCertChainPath = "/some/path/client.pem";
    config.clientKeyPath       = "/some/path/client.key";
    stream                     = SolidSyslogTlsStream_Create(&streamStorage, &config);
    OpenSslFake_SetUsePrivateKeyFileFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, UsePrivateKeyFileFailureFreesCtx)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.clientCertChainPath = "/some/path/client.pem";
    config.clientKeyPath       = "/some/path/client.key";
    stream                     = SolidSyslogTlsStream_Create(&streamStorage, &config);
    OpenSslFake_SetUsePrivateKeyFileFails(true);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, OpenSslFake_CtxFreeCallCount());
}

TEST(SolidSyslogTlsStream, OpenReturnsFalseWhenCheckPrivateKeyFails)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.clientCertChainPath = "/some/path/client.pem";
    config.clientKeyPath       = "/some/path/client.key";
    stream                     = SolidSyslogTlsStream_Create(&streamStorage, &config);
    OpenSslFake_SetCheckPrivateKeyFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogTlsStream, CheckPrivateKeyFailureFreesCtx)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.clientCertChainPath = "/some/path/client.pem";
    config.clientKeyPath       = "/some/path/client.key";
    stream                     = SolidSyslogTlsStream_Create(&streamStorage, &config);
    OpenSslFake_SetCheckPrivateKeyFails(true);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, OpenSslFake_CtxFreeCallCount());
}

TEST(SolidSyslogTlsStream, OpenPassesCtxFromNewToUseCertChainFile)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.clientCertChainPath = "/some/path/client.pem";
    config.clientKeyPath       = "/some/path/client.key";
    stream                     = SolidSyslogTlsStream_Create(&streamStorage, &config);
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastCtxReturned(), OpenSslFake_LastUseCertChainFileCtxArg());
}

TEST(SolidSyslogTlsStream, OpenPassesCtxFromNewToUsePrivateKeyFile)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.clientCertChainPath = "/some/path/client.pem";
    config.clientKeyPath       = "/some/path/client.key";
    stream                     = SolidSyslogTlsStream_Create(&streamStorage, &config);
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastCtxReturned(), OpenSslFake_LastUsePrivateKeyFileCtxArg());
}

TEST(SolidSyslogTlsStream, OpenPassesCtxFromNewToCheckPrivateKey)
{
    SolidSyslogTlsStream_Destroy(stream);
    config.clientCertChainPath = "/some/path/client.pem";
    config.clientKeyPath       = "/some/path/client.key";
    stream                     = SolidSyslogTlsStream_Create(&streamStorage, &config);
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(OpenSslFake_LastCtxReturned(), OpenSslFake_LastCheckPrivateKeyCtxArg());
}

TEST(SolidSyslogTlsStream, DefaultPortMatchesRfc5425)
{
    LONGS_EQUAL(6514, SOLIDSYSLOG_TLS_DEFAULT_PORT);
}
