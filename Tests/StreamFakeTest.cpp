#include "AddressFake.h"
#include "SolidSyslogStream.h"
#include "StreamFake.h"
#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

// clang-format off
TEST_GROUP(StreamFake)
{
    struct SolidSyslogStream* stream = nullptr;

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
    struct SolidSyslogAddress* addr = AddressFake_Get();
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE_ON(StreamFake_Open, stream, ONCE);
}

TEST(StreamFake, OpenCapturesAddr)
{
    struct SolidSyslogAddress* addr = AddressFake_Get();
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(addr, StreamFake_LastOpenAddr(stream));
}

TEST(StreamFake, SendIncrementsCount)
{
    const char payload[] = "hi";
    SolidSyslogStream_Send(stream, payload, sizeof(payload));
    CALLED_FAKE_ON(StreamFake_Send, stream, ONCE);
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
    CALLED_FAKE_ON(StreamFake_Read, stream, ONCE);
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
    char buf[16];
    SolidSyslogSsize n = SolidSyslogStream_Read(stream, buf, sizeof(buf));
    LONGS_EQUAL(5, n);
}

TEST(StreamFake, CloseIncrementsCount)
{
    SolidSyslogStream_Close(stream);
    CALLED_FAKE_ON(StreamFake_Close, stream, ONCE);
}

TEST(StreamFake, OpenDefaultsToSuccess)
{
    struct SolidSyslogAddress* addr = AddressFake_Get();
    CHECK_TRUE(SolidSyslogStream_Open(stream, addr));
}

TEST(StreamFake, SetOpenFailsMakesOpenReturnFalse)
{
    StreamFake_SetOpenFails(stream, true);
    struct SolidSyslogAddress* addr = AddressFake_Get();
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(StreamFake, NoFrameBeforeAnySend)
{
    LONGS_EQUAL(0, StreamFake_FrameCount(stream));
}

TEST(StreamFake, AFrameSentAsPrefixThenBodyIsKeptWhole)
{
    SolidSyslogStream_Send(stream, "5 ", 2);
    SolidSyslogStream_Send(stream, "hello", 5);
    LONGS_EQUAL(1, StreamFake_FrameCount(stream));
    STRCMP_EQUAL("hello", StreamFake_Frame(stream, 0));
}

TEST(StreamFake, FramesAreKeptInTheOrderTheyCompleted)
{
    SolidSyslogStream_Send(stream, "3 one", 5);
    SolidSyslogStream_Send(stream, "3 two", 5);
    LONGS_EQUAL(2, StreamFake_FrameCount(stream));
    STRCMP_EQUAL("one", StreamFake_Frame(stream, 0));
    STRCMP_EQUAL("two", StreamFake_Frame(stream, 1));
}

TEST(StreamFake, AFailedSendKeepsNoBytes)
{
    StreamFake_SetSendFails(stream, true);
    SolidSyslogStream_Send(stream, "5 hello", 7);
    LONGS_EQUAL(0, StreamFake_FrameCount(stream));
}

TEST(StreamFake, CloseDropsTheFrameInProgress)
{
    SolidSyslogStream_Send(stream, "5 he", 4);
    SolidSyslogStream_Close(stream);
    SolidSyslogStream_Send(stream, "3 abc", 5);
    LONGS_EQUAL(1, StreamFake_FrameCount(stream));
    STRCMP_EQUAL("abc", StreamFake_Frame(stream, 0));
}

TEST(StreamFake, AZeroLengthFrameCompletesAtItsPrefix)
{
    SolidSyslogStream_Send(stream, "0 ", 2);
    LONGS_EQUAL(1, StreamFake_FrameCount(stream));
    STRCMP_EQUAL("", StreamFake_Frame(stream, 0));
}
