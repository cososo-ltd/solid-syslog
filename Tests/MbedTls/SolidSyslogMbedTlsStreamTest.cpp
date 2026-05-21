#include "CppUTest/TestHarness.h"

extern "C"
{
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ssl.h>

#include "MbedTlsFake.h"
#include "SolidSyslogAddress.h"
#include "SolidSyslogMbedTlsStream.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"
#include "StreamFake.h"
}

namespace
{
void NoOpSleep(int milliseconds)
{
    (void) milliseconds;
}
} // namespace

// clang-format off
TEST_GROUP(SolidSyslogMbedTlsStream)
{
    struct SolidSyslogStream*         transport = nullptr;
    struct SolidSyslogStream*         handle    = nullptr;
    struct SolidSyslogMbedTlsStreamConfig config = {};

    void setup() override
    {
        MbedTlsFake_Reset();
        transport = StreamFake_Create();
        config.Transport = transport;
        config.Sleep = NoOpSleep;
        handle = SolidSyslogMbedTlsStream_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogMbedTlsStream_Destroy(handle);
        StreamFake_Destroy(transport);
    }

    /* Tests needing config tweaks (CaChain, Rng, ServerName, …) call this
     * to release setup()'s pool slot, mutate `config`, then re-Create. */
    void ReCreateHandleWithUpdatedConfig()
    {
        SolidSyslogMbedTlsStream_Destroy(handle);
        handle = SolidSyslogMbedTlsStream_Create(&config);
    }
};

// clang-format on

TEST(SolidSyslogMbedTlsStream, OpenDelegatesToInjectedTransport)
{
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, StreamFake_OpenCallCount(transport));
    POINTERS_EQUAL(addr, StreamFake_LastOpenAddr(transport));
}

TEST(SolidSyslogMbedTlsStream, OpenInitialisesSslConfig)
{
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, MbedTlsFake_SslConfigInitCallCount());
}

TEST(SolidSyslogMbedTlsStream, OpenAppliesClientStreamDefaultsToSslConfig)
{
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, MbedTlsFake_SslConfigDefaultsCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslConfigInitArg(), MbedTlsFake_LastSslConfigDefaultsConfigArg());
    LONGS_EQUAL(MBEDTLS_SSL_IS_CLIENT, MbedTlsFake_LastSslConfigDefaultsEndpoint());
    LONGS_EQUAL(MBEDTLS_SSL_TRANSPORT_STREAM, MbedTlsFake_LastSslConfigDefaultsTransport());
    LONGS_EQUAL(MBEDTLS_SSL_PRESET_DEFAULT, MbedTlsFake_LastSslConfigDefaultsPreset());
}

TEST(SolidSyslogMbedTlsStream, OpenInitialisesSslContext)
{
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, MbedTlsFake_SslInitCallCount());
}

TEST(SolidSyslogMbedTlsStream, OpenSetupBindsSslContextToSslConfig)
{
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, MbedTlsFake_SslSetupCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslInitArg(), MbedTlsFake_LastSslSetupContextArg());
    POINTERS_EQUAL(MbedTlsFake_LastSslConfigInitArg(), MbedTlsFake_LastSslSetupConfigArg());
}

TEST(SolidSyslogMbedTlsStream, OpenWiresBioWithNonNullSendRecvAndNullRecvTimeout)
{
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, MbedTlsFake_SslSetBioCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslInitArg(), MbedTlsFake_LastSslSetBioContextArg());
    CHECK(MbedTlsFake_LastSslSetBioPBioArg() != nullptr);
    CHECK(MbedTlsFake_LastSslSetBioSendCallback() != nullptr);
    CHECK(MbedTlsFake_LastSslSetBioRecvCallback() != nullptr);
}

TEST(SolidSyslogMbedTlsStream, OpenDrivesHandshakeOnTheSslContext)
{
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, MbedTlsFake_SslHandshakeCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslInitArg(), MbedTlsFake_LastSslHandshakeArg());
}

TEST(SolidSyslogMbedTlsStream, OpenReturnsTrueWhenHandshakeSucceeds)
{
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);
    MbedTlsFake_SetSslHandshakeReturn(0);

    CHECK_TRUE(SolidSyslogStream_Open(handle, addr));
}

TEST(SolidSyslogMbedTlsStream, OpenReturnsFalseWhenHandshakeFails)
{
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);
    MbedTlsFake_SetSslHandshakeReturn(-1);

    CHECK_FALSE(SolidSyslogStream_Open(handle, addr));
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

TEST(SolidSyslogMbedTlsStream, CloseSendsSslCloseNotifyOnTheSslContextFromOpen)
{
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);
    SolidSyslogStream_Open(handle, addr);

    SolidSyslogStream_Close(handle);

    LONGS_EQUAL(1, MbedTlsFake_SslCloseNotifyCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslInitArg(), MbedTlsFake_LastSslCloseNotifyArg());
}

TEST(SolidSyslogMbedTlsStream, CloseFreesSslContextAndSslConfigFromOpen)
{
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);
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
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);
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
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);
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

TEST(SolidSyslogMbedTlsStream, OpenSetsAuthmodeRequired)
{
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);

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
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);

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
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);

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
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(1, MbedTlsFake_SslSetHostnameCallCount());
    POINTERS_EQUAL(MbedTlsFake_LastSslInitArg(), MbedTlsFake_LastSslSetHostnameContextArg());
    STRCMP_EQUAL("syslog.example.com", MbedTlsFake_LastSslSetHostnameNameArg());
}

TEST(SolidSyslogMbedTlsStream, OpenSkipsHostnameWhenServerNameIsNull)
{
    /* setup() left config.ServerName at NULL. */
    SolidSyslogAddressStorage storage = {};
    struct SolidSyslogAddress* addr = SolidSyslogAddress_FromStorage(&storage);

    SolidSyslogStream_Open(handle, addr);

    LONGS_EQUAL(0, MbedTlsFake_SslSetHostnameCallCount());
}
