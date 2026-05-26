#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/prov_ssl.h>
#include <openssl/types.h>

#include "OpenSslFake.h"
#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

// clang-format off
TEST_GROUP(OpenSslFake)
{
    void setup() override { OpenSslFake_Reset(); }
};
// clang-format on

TEST(OpenSslFake, CtxNewCountIsZeroAfterReset)
{
    CALLED_FAKE(OpenSslFake_CtxNew, NEVER);
}

TEST(OpenSslFake, CtxNewIncrementsCount)
{
    SSL_CTX_new(TLS_client_method());
    CALLED_FAKE(OpenSslFake_CtxNew, ONCE);
}

TEST(OpenSslFake, CtxNewReturnsNonNull)
{
    CHECK_TRUE(SSL_CTX_new(TLS_client_method()) != nullptr);
}

TEST(OpenSslFake, LoadVerifyLocationsCapturesCaPath)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_load_verify_locations(ctx, "/my/ca.pem", nullptr);
    STRCMP_EQUAL("/my/ca.pem", OpenSslFake_LastCaBundlePath());
}

TEST(OpenSslFake, SetVerifyCapturesMode)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
    LONGS_EQUAL(SSL_VERIFY_PEER, OpenSslFake_LastVerifyMode());
}

TEST(OpenSslFake, SetMinProtoVersionCapturesVersion)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
    LONGS_EQUAL(TLS1_2_VERSION, OpenSslFake_LastMinProtoVersion());
}

TEST(OpenSslFake, CtxNewReturnValueIsSurfaced)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    POINTERS_EQUAL(ctx, OpenSslFake_LastCtxReturned());
}

TEST(OpenSslFake, SslNewIncrementsCount)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL_new(ctx);
    CALLED_FAKE(OpenSslFake_SslNew, ONCE);
}

TEST(OpenSslFake, SslNewReturnsNonNull)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    CHECK_TRUE(SSL_new(ctx) != nullptr);
}

TEST(OpenSslFake, SslNewCapturesCtxArg)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL_new(ctx);
    POINTERS_EQUAL(ctx, OpenSslFake_LastSslNewCtxArg());
}

TEST(OpenSslFake, BioNewIncrementsCount)
{
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO_new(method);
    CALLED_FAKE(OpenSslFake_BioNew, ONCE);
}

TEST(OpenSslFake, BioNewReturnsNonNull)
{
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    CHECK_TRUE(BIO_new(method) != nullptr);
}

TEST(OpenSslFake, BioNewReturnValueIsSurfaced)
{
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO* bio = BIO_new(method);
    POINTERS_EQUAL(bio, OpenSslFake_LastBioReturned());
}

TEST(OpenSslFake, SetBioIncrementsCount)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO* bio = BIO_new(method);
    SSL_set_bio(ssl, bio, bio);
    CALLED_FAKE(OpenSslFake_SetBio, ONCE);
}

TEST(OpenSslFake, SetBioCapturesSslArg)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO* bio = BIO_new(method);
    SSL_set_bio(ssl, bio, bio);
    POINTERS_EQUAL(ssl, OpenSslFake_LastSetBioSslArg());
}

TEST(OpenSslFake, SetBioCapturesReadBioArg)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO* bio = BIO_new(method);
    SSL_set_bio(ssl, bio, bio);
    POINTERS_EQUAL(bio, OpenSslFake_LastSetBioReadBioArg());
}

TEST(OpenSslFake, ConnectIncrementsCount)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    SSL_connect(ssl);
    CALLED_FAKE(OpenSslFake_Connect, ONCE);
}

TEST(OpenSslFake, ConnectCapturesSslArg)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    SSL_connect(ssl);
    POINTERS_EQUAL(ssl, OpenSslFake_LastConnectSslArg());
}

TEST(OpenSslFake, ConnectDefaultsToSuccess)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    LONGS_EQUAL(1, SSL_connect(ssl));
}

TEST(OpenSslFake, SetTlsExtHostNameCapturesHostname)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    SSL_set_tlsext_host_name(ssl, "host.example");
    STRCMP_EQUAL("host.example", OpenSslFake_LastSniHostname());
}

TEST(OpenSslFake, Set1HostCapturesHostname)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    SSL_set1_host(ssl, "host.example");
    STRCMP_EQUAL("host.example", OpenSslFake_LastSet1Host());
}

TEST(OpenSslFake, BioSetDataCapturesData)
{
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO* bio = BIO_new(method);
    int sentinel = 0;
    BIO_set_data(bio, &sentinel);
    POINTERS_EQUAL(&sentinel, OpenSslFake_LastSetDataArg());
}

TEST(OpenSslFake, BioGetDataReturnsPreviouslySetData)
{
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO* bio = BIO_new(method);
    int sentinel = 0;
    BIO_set_data(bio, &sentinel);
    POINTERS_EQUAL(&sentinel, BIO_get_data(bio));
}

TEST(OpenSslFake, BioDataIsStoredPerInstance)
{
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO* bio1 = BIO_new(method);
    BIO* bio2 = BIO_new(method);
    int a = 0;
    int b = 0;
    BIO_set_data(bio1, &a);
    BIO_set_data(bio2, &b);
    POINTERS_EQUAL(&a, BIO_get_data(bio1));
    POINTERS_EQUAL(&b, BIO_get_data(bio2));
}

// NOLINTNEXTLINE(readability-non-const-parameter) -- signature fixed by OpenSSL BIO_meth_set_read contract
static int DummyRead(BIO* bio, char* buf, int size)
{
    (void) bio;
    (void) buf;
    (void) size;
    return 0;
}

TEST(OpenSslFake, BioMethSetReadCapturesCallback)
{
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO_meth_set_read(method, DummyRead);
    FUNCTIONPOINTERS_EQUAL(DummyRead, OpenSslFake_LastBioReadCallback());
}

static int DummyWrite(BIO* bio, const char* buf, int size)
{
    (void) bio;
    (void) buf;
    (void) size;
    return 0;
}

TEST(OpenSslFake, BioMethSetWriteCapturesCallback)
{
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO_meth_set_write(method, DummyWrite);
    FUNCTIONPOINTERS_EQUAL(DummyWrite, OpenSslFake_LastBioWriteCallback());
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- signature fixed by OpenSSL BIO_meth_set_ctrl contract
static long DummyCtrl(BIO* bio, int cmd, long larg, void* parg)
{
    (void) bio;
    (void) cmd;
    (void) larg;
    (void) parg;
    return 0;
}

TEST(OpenSslFake, BioMethSetCtrlCapturesCallback)
{
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO_meth_set_ctrl(method, DummyCtrl);
    FUNCTIONPOINTERS_EQUAL(DummyCtrl, OpenSslFake_LastBioCtrlCallback());
}

static int DummyCreate(BIO* bio)
{
    (void) bio;
    return 1;
}

TEST(OpenSslFake, BioMethSetCreateCapturesCallback)
{
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO_meth_set_create(method, DummyCreate);
    FUNCTIONPOINTERS_EQUAL(DummyCreate, OpenSslFake_LastBioCreateCallback());
}

TEST(OpenSslFake, WriteIncrementsCount)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    SSL_write(ssl, "x", 1);
    CALLED_FAKE(OpenSslFake_Write, ONCE);
}

TEST(OpenSslFake, WriteCapturesSslArg)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    SSL_write(ssl, "x", 1);
    POINTERS_EQUAL(ssl, OpenSslFake_LastWriteSslArg());
}

TEST(OpenSslFake, WriteCapturesBuffer)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    const char* msg = "payload";
    SSL_write(ssl, msg, 7);
    POINTERS_EQUAL(msg, OpenSslFake_LastWriteBuf());
}

TEST(OpenSslFake, WriteCapturesSize)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    SSL_write(ssl, "payload", 7);
    LONGS_EQUAL(7, OpenSslFake_LastWriteSize());
}

TEST(OpenSslFake, WriteDefaultsToEchoingSize)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    LONGS_EQUAL(7, SSL_write(ssl, "payload", 7));
}

TEST(OpenSslFake, ShutdownIncrementsCount)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    SSL_shutdown(ssl);
    CALLED_FAKE(OpenSslFake_Shutdown, ONCE);
}

TEST(OpenSslFake, FreeIncrementsCount)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    SSL_free(ssl);
    CALLED_FAKE(OpenSslFake_Free, ONCE);
}

TEST(OpenSslFake, CtxFreeIncrementsCount)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_free(ctx);
    CALLED_FAKE(OpenSslFake_CtxFree, ONCE);
}

/* -------------------------------------------------------------------------
 * Arg captures — prove each fake function records the args its callers pass.
 * ------------------------------------------------------------------------- */

TEST(OpenSslFake, CtxNewCapturesMethodArg)
{
    const SSL_METHOD* method = TLS_client_method();
    SSL_CTX_new(method);
    POINTERS_EQUAL(method, OpenSslFake_LastCtxNewMethodArg());
}

TEST(OpenSslFake, LoadVerifyLocationsCapturesCtxArg)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_load_verify_locations(ctx, "/ca.pem", nullptr);
    POINTERS_EQUAL(ctx, OpenSslFake_LastLoadVerifyLocationsCtxArg());
}

TEST(OpenSslFake, SetVerifyCapturesCtxArg)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
    POINTERS_EQUAL(ctx, OpenSslFake_LastSetVerifyCtxArg());
}

TEST(OpenSslFake, SetMinProtoVersionCapturesCtxArg)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
    POINTERS_EQUAL(ctx, OpenSslFake_LastSslCtxCtrlCtxArg());
}

TEST(OpenSslFake, BioMethSetReadCapturesMethodArg)
{
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO_meth_set_read(method, DummyRead);
    POINTERS_EQUAL(method, OpenSslFake_LastBioMethSetReadMethodArg());
}

TEST(OpenSslFake, BioMethSetWriteCapturesMethodArg)
{
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO_meth_set_write(method, DummyWrite);
    POINTERS_EQUAL(method, OpenSslFake_LastBioMethSetWriteMethodArg());
}

TEST(OpenSslFake, BioNewCapturesMethodArg)
{
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO_new(method);
    POINTERS_EQUAL(method, OpenSslFake_LastBioNewMethodArg());
}

TEST(OpenSslFake, BioMethNewReturnValueIsSurfaced)
{
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    POINTERS_EQUAL(method, OpenSslFake_LastBioMethReturned());
}

TEST(OpenSslFake, BioSetDataCapturesBioArg)
{
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO* bio = BIO_new(method);
    int sentinel = 0;
    BIO_set_data(bio, &sentinel);
    POINTERS_EQUAL(bio, OpenSslFake_LastSetDataBioArg());
}

TEST(OpenSslFake, BioGetDataCapturesBioArg)
{
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO* bio = BIO_new(method);
    BIO_get_data(bio);
    POINTERS_EQUAL(bio, OpenSslFake_LastGetDataBioArg());
}

TEST(OpenSslFake, SetBioCapturesWriteBioArg)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    BIO_METHOD* method = BIO_meth_new(0, "fake");
    BIO* rbio = BIO_new(method);
    BIO* wbio = BIO_new(method);
    SSL_set_bio(ssl, rbio, wbio);
    POINTERS_EQUAL(wbio, OpenSslFake_LastSetBioWriteBioArg());
}

TEST(OpenSslFake, SetTlsExtHostNameCapturesSslArg)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    SSL_set_tlsext_host_name(ssl, "host.example");
    POINTERS_EQUAL(ssl, OpenSslFake_LastSslCtrlSslArg());
}

TEST(OpenSslFake, Set1HostCapturesSslArg)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    SSL_set1_host(ssl, "host.example");
    POINTERS_EQUAL(ssl, OpenSslFake_LastSet1HostSslArg());
}

TEST(OpenSslFake, ShutdownCapturesSslArg)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    SSL_shutdown(ssl);
    POINTERS_EQUAL(ssl, OpenSslFake_LastShutdownSslArg());
}

TEST(OpenSslFake, FreeCapturesSslArg)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    SSL_free(ssl);
    POINTERS_EQUAL(ssl, OpenSslFake_LastFreeSslArg());
}

TEST(OpenSslFake, CtxFreeCapturesCtxArg)
{
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_free(ctx);
    POINTERS_EQUAL(ctx, OpenSslFake_LastCtxFreeCtxArg());
}

/* -------------------------------------------------------------------------
 * Failure-mode switches — tests opt into failure returns via SetXxxFails.
 * ------------------------------------------------------------------------- */

TEST(OpenSslFake, SetConnectFailsMakesConnectReturnNegative)
{
    OpenSslFake_SetConnectFails(true);
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    CHECK_TRUE(SSL_connect(ssl) <= 0);
}

TEST(OpenSslFake, SetWriteFailsMakesWriteReturnNegative)
{
    OpenSslFake_SetWriteFails(true);
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    SSL* ssl = SSL_new(ctx);
    CHECK_TRUE(SSL_write(ssl, "x", 1) <= 0);
}
