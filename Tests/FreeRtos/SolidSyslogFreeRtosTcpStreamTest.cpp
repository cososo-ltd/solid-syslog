#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
                               // macros

#include "SolidSyslogAddress.h"
#include "SolidSyslogFreeRtosTcpStream.h"
#include "SolidSyslogStream.h"

#include "FreeRtosSocketsFake.h"

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

// clang-format off
TEST_GROUP(SolidSyslogFreeRtosTcpStream)
{
    SolidSyslogFreeRtosTcpStreamStorage storage{};
    struct SolidSyslogStream*           stream = nullptr;
    SolidSyslogAddressStorage           addrStorage{};
    struct SolidSyslogAddress*          addr = nullptr;

    void setup() override
    {
        FreeRtosSocketsFake_Reset();
        stream = SolidSyslogFreeRtosTcpStream_Create(&storage);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- char-type aliasing into platform layout, storage is intptr_t-aligned
        auto* sin                  = reinterpret_cast<struct freertos_sockaddr*>(&addrStorage);
        sin->sin_family            = FREERTOS_AF_INET;
        sin->sin_port              = FreeRTOS_htons(514);
        sin->sin_address.ulIP_IPv4 = FreeRTOS_inet_addr_quick(10, 0, 2, 2);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- platform-layout cast, see above
        addr = reinterpret_cast<struct SolidSyslogAddress*>(&addrStorage);
    }

    void teardown() override
    {
        SolidSyslogFreeRtosTcpStream_Destroy(stream);
    }
};

// clang-format on

TEST(SolidSyslogFreeRtosTcpStream, CreateReturnsNonNullStream)
{
    CHECK(stream != nullptr);
}

TEST(SolidSyslogFreeRtosTcpStream, OpenCreatesTcpSocket)
{
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(FreeRtosSocketsFake_Socket, ONCE);
    LONGS_EQUAL(FREERTOS_AF_INET, FreeRtosSocketsFake_LastSocketDomain());
    LONGS_EQUAL(FREERTOS_SOCK_STREAM, FreeRtosSocketsFake_LastSocketType());
    LONGS_EQUAL(FREERTOS_IPPROTO_TCP, FreeRtosSocketsFake_LastSocketProtocol());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenReturnsTrueOnSuccess)
{
    CHECK_TRUE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogFreeRtosTcpStream, OpenReturnsFalseWhenSocketFails)
{
    FreeRtosSocketsFake_SetSocketFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogFreeRtosTcpStream, OpenSetsConnectTimeoutBeforeConnect)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(pdMS_TO_TICKS(200), FreeRtosSocketsFake_SndTimeoAtConnect());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenCallsConnectWithSocketAndAddress)
{
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(FreeRtosSocketsFake_Connect, ONCE);
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastConnectSocket());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- platform-layout cast, see setup
    POINTERS_EQUAL(reinterpret_cast<const struct freertos_sockaddr*>(addr), FreeRtosSocketsFake_LastConnectAddress());
    LONGS_EQUAL(sizeof(struct freertos_sockaddr), FreeRtosSocketsFake_LastConnectAddressLength());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenClearsSendTimeoutAfterConnect)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(0, FreeRtosSocketsFake_LastSndTimeoSet());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenClearsRecvTimeoutAfterConnect)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(0, FreeRtosSocketsFake_LastRcvTimeoSet());
    LONGS_EQUAL(1, FreeRtosSocketsFake_RcvTimeoSetCallCount());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenCallsSetsockoptWithReturnedSocketAndLevelZero)
{
    SolidSyslogStream_Open(stream, addr);
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastSetsockoptSocket());
    LONGS_EQUAL(0, FreeRtosSocketsFake_LastSetsockoptLevel());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenPassesTickTypeSizedOptionLengthToSetsockopt)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(sizeof(TickType_t), FreeRtosSocketsFake_LastSetsockoptOptionLength());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenReturnsFalseOnConnectFailure)
{
    FreeRtosSocketsFake_SetConnectFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogFreeRtosTcpStream, OpenClosesSocketOnConnectFailure)
{
    FreeRtosSocketsFake_SetConnectFails(true);
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastClosesocketSocket());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenIsIdempotent)
{
    SolidSyslogStream_Open(stream, addr);
    CHECK_TRUE(SolidSyslogStream_Open(stream, addr));
    CALLED_FAKE(FreeRtosSocketsFake_Socket, ONCE);
    CALLED_FAKE(FreeRtosSocketsFake_Connect, ONCE);
}

TEST(SolidSyslogFreeRtosTcpStream, SendFailsBeforeOpen)
{
    CHECK_FALSE(SolidSyslogStream_Send(stream, "x", 1));
    CALLED_FAKE(FreeRtosSocketsFake_Send, NEVER);
}

TEST(SolidSyslogFreeRtosTcpStream, SendCallsFreeRtosSendWithSocketBufferAndLength)
{
    static const char TEST_MESSAGE[] = "hello";
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, sizeof(TEST_MESSAGE) - 1);
    CALLED_FAKE(FreeRtosSocketsFake_Send, ONCE);
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastSendSocket());
    POINTERS_EQUAL(TEST_MESSAGE, FreeRtosSocketsFake_LastSendBuffer());
    LONGS_EQUAL(sizeof(TEST_MESSAGE) - 1, FreeRtosSocketsFake_LastSendLength());
    LONGS_EQUAL(0, FreeRtosSocketsFake_LastSendFlags());
}

TEST(SolidSyslogFreeRtosTcpStream, SendReturnsTrueOnFullWrite)
{
    SolidSyslogStream_Open(stream, addr);
    CHECK_TRUE(SolidSyslogStream_Send(stream, "hello", 5));
}

TEST(SolidSyslogFreeRtosTcpStream, SendReturnsFalseOnShortWrite)
{
    SolidSyslogStream_Open(stream, addr);
    FreeRtosSocketsFake_SetSendReturn(3);
    CHECK_FALSE(SolidSyslogStream_Send(stream, "hello", 5));
}

TEST(SolidSyslogFreeRtosTcpStream, SendDoesNotRetryAfterShortWrite)
{
    SolidSyslogStream_Open(stream, addr);
    FreeRtosSocketsFake_SetSendReturn(3);
    SolidSyslogStream_Send(stream, "hello", 5);
    CALLED_FAKE(FreeRtosSocketsFake_Send, ONCE);
}

TEST(SolidSyslogFreeRtosTcpStream, SendClosesSocketOnShortWrite)
{
    SolidSyslogStream_Open(stream, addr);
    FreeRtosSocketsFake_SetSendReturn(3);
    SolidSyslogStream_Send(stream, "hello", 5);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastClosesocketSocket());
}

TEST(SolidSyslogFreeRtosTcpStream, SendReturnsFalseOnError)
{
    SolidSyslogStream_Open(stream, addr);
    FreeRtosSocketsFake_SetSendFails(true);
    CHECK_FALSE(SolidSyslogStream_Send(stream, "hello", 5));
}

TEST(SolidSyslogFreeRtosTcpStream, SendClosesSocketOnError)
{
    SolidSyslogStream_Open(stream, addr);
    FreeRtosSocketsFake_SetSendFails(true);
    SolidSyslogStream_Send(stream, "hello", 5);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
}

TEST(SolidSyslogFreeRtosTcpStream, ReadReturnsNegativeOneBeforeOpen)
{
    char buf[16];
    LONGS_EQUAL(-1, SolidSyslogStream_Read(stream, buf, sizeof(buf)));
    CALLED_FAKE(FreeRtosSocketsFake_Recv, NEVER);
}

TEST(SolidSyslogFreeRtosTcpStream, ReadCallsFreeRtosRecvWithSocketBufferAndLength)
{
    char buf[16];
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    CALLED_FAKE(FreeRtosSocketsFake_Recv, ONCE);
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastRecvSocket());
    POINTERS_EQUAL(buf, FreeRtosSocketsFake_LastRecvBuffer());
    LONGS_EQUAL(sizeof(buf), FreeRtosSocketsFake_LastRecvLength());
    LONGS_EQUAL(0, FreeRtosSocketsFake_LastRecvFlags());
}

TEST(SolidSyslogFreeRtosTcpStream, ReadReturnsBytesOnSuccess)
{
    char buf[16];
    SolidSyslogStream_Open(stream, addr);
    FreeRtosSocketsFake_SetRecvReturn(7);
    LONGS_EQUAL(7, SolidSyslogStream_Read(stream, buf, sizeof(buf)));
}

TEST(SolidSyslogFreeRtosTcpStream, ReadReturnsZeroWhenWouldBlock)
{
    char buf[16];
    SolidSyslogStream_Open(stream, addr);
    FreeRtosSocketsFake_SetRecvReturn(0);
    LONGS_EQUAL(0, SolidSyslogStream_Read(stream, buf, sizeof(buf)));
}

TEST(SolidSyslogFreeRtosTcpStream, ReadLeavesSocketOpenWhenWouldBlock)
{
    char buf[16];
    SolidSyslogStream_Open(stream, addr);
    FreeRtosSocketsFake_SetRecvReturn(0);
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, NEVER);
}

TEST(SolidSyslogFreeRtosTcpStream, ReadReturnsNegativeOneOnErrorAndClosesSocket)
{
    char buf[16];
    SolidSyslogStream_Open(stream, addr);
    FreeRtosSocketsFake_SetRecvFails(true);
    LONGS_EQUAL(-1, SolidSyslogStream_Read(stream, buf, sizeof(buf)));
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
}

TEST(SolidSyslogFreeRtosTcpStream, CloseWithoutOpenIsNoOp)
{
    SolidSyslogStream_Close(stream);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, NEVER);
}

TEST(SolidSyslogFreeRtosTcpStream, CloseClosesOpenSocket)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastClosesocketSocket());
}

TEST(SolidSyslogFreeRtosTcpStream, CloseIsIdempotent)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    SolidSyslogStream_Close(stream);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
}

TEST(SolidSyslogFreeRtosTcpStream, DestroyClosesOpenSocket)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogFreeRtosTcpStream_Destroy(stream);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
}

TEST(SolidSyslogFreeRtosTcpStream, DestroyAfterCloseDoesNotCloseAgain)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    SolidSyslogFreeRtosTcpStream_Destroy(stream);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
}
