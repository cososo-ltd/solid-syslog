#include "SolidSyslogAddress.h"
#include "SolidSyslogStream.h"
#include "StreamFake.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(StreamFake)
{
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogStream* stream = nullptr;

    // cppcheck-suppress unreadVariable -- read by teardown and tests; cppcheck does not model CppUTest lifecycle
    void setup() override { stream = StreamFake_Create(); }
    void teardown() override { StreamFake_Destroy(stream); }
};

// clang-format on

TEST(StreamFake, CreateSucceeds)
{
    CHECK_TRUE(stream != nullptr);
}

TEST(StreamFake, OpenIncrementsCount)
{
    SolidSyslogAddressStorage  storage = {0};
    struct SolidSyslogAddress* addr    = SolidSyslogAddress_FromStorage(&storage);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, StreamFake_OpenCallCount(stream));
}

TEST(StreamFake, OpenCapturesAddr)
{
    SolidSyslogAddressStorage  storage = {0};
    struct SolidSyslogAddress* addr    = SolidSyslogAddress_FromStorage(&storage);
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(addr, StreamFake_LastOpenAddr(stream));
}

TEST(StreamFake, SendIncrementsCount)
{
    const char payload[] = "hi";
    SolidSyslogStream_Send(stream, payload, sizeof(payload));
    LONGS_EQUAL(1, StreamFake_SendCallCount(stream));
}

TEST(StreamFake, SendCapturesBuffer)
{
    const char payload[] = "hi";
    SolidSyslogStream_Send(stream, payload, sizeof(payload));
    POINTERS_EQUAL(payload, StreamFake_LastSendBuf(stream));
}

TEST(StreamFake, SendCapturesSize)
{
    const char payload[] = "hi";
    SolidSyslogStream_Send(stream, payload, sizeof(payload));
    LONGS_EQUAL(sizeof(payload), StreamFake_LastSendSize(stream));
}

TEST(StreamFake, ReadIncrementsCount)
{
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    LONGS_EQUAL(1, StreamFake_ReadCallCount(stream));
}

TEST(StreamFake, ReadCapturesBuffer)
{
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    POINTERS_EQUAL(buf, StreamFake_LastReadBuf(stream));
}

TEST(StreamFake, ReadCapturesSize)
{
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    LONGS_EQUAL(sizeof(buf), StreamFake_LastReadSize(stream));
}

TEST(StreamFake, ReadReturnsConfiguredValue)
{
    StreamFake_SetReadReturn(stream, 5);
    char             buf[16];
    SolidSyslogSsize n = SolidSyslogStream_Read(stream, buf, sizeof(buf));
    LONGS_EQUAL(5, n);
}

TEST(StreamFake, CloseIncrementsCount)
{
    SolidSyslogStream_Close(stream);
    LONGS_EQUAL(1, StreamFake_CloseCallCount(stream));
}

TEST(StreamFake, OpenDefaultsToSuccess)
{
    SolidSyslogAddressStorage  storage = {0};
    struct SolidSyslogAddress* addr    = SolidSyslogAddress_FromStorage(&storage);
    CHECK_TRUE(SolidSyslogStream_Open(stream, addr));
}

TEST(StreamFake, SetOpenFailsMakesOpenReturnFalse)
{
    StreamFake_SetOpenFails(stream, true);
    SolidSyslogAddressStorage  storage = {0};
    struct SolidSyslogAddress* addr    = SolidSyslogAddress_FromStorage(&storage);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}
