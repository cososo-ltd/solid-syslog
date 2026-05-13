#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
                               // macros

#include "SolidSyslogAddress.h"
#include "SolidSyslogFreeRtosTcpStream.h"
#include "SolidSyslogStream.h"

#include "FreeRtosArpFake.h"
#include "FreeRtosSocketsFake.h"
#include "FreeRtosTaskFake.h"

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

static const uint16_t   TEST_PORT              = 514;
static const char       TEST_MESSAGE[]         = "hello";
static const size_t     TEST_MESSAGE_LEN       = sizeof(TEST_MESSAGE) - 1U;
static const BaseType_t TEST_SHORT_WRITE_BYTES = 3;
static const BaseType_t TEST_READ_BYTES        = 7;

// clang-format off
TEST_GROUP(SolidSyslogFreeRtosTcpStream)
{
    SolidSyslogFreeRtosTcpStreamStorage storage{};
    struct SolidSyslogStream*           stream = nullptr;
    SolidSyslogAddressStorage           addrStorage{};
    struct SolidSyslogAddress*          addr = nullptr;
    char                                readBuffer[16] = {0};

    void setup() override
    {
        FreeRtosSocketsFake_Reset();
        FreeRtosArpFake_Reset();
        FreeRtosTaskFake_Reset();
        stream = SolidSyslogFreeRtosTcpStream_Create(&storage);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- char-type aliasing into platform layout, storage is intptr_t-aligned
        auto* sin                  = reinterpret_cast<struct freertos_sockaddr*>(&addrStorage);
        sin->sin_family            = FREERTOS_AF_INET;
        sin->sin_port              = FreeRTOS_htons(TEST_PORT);
        sin->sin_address.ulIP_IPv4 = FreeRTOS_inet_addr_quick(10, 0, 2, 2);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- platform-layout cast, see above
        addr = reinterpret_cast<struct SolidSyslogAddress*>(&addrStorage);
    }

    void teardown() override
    {
        SolidSyslogFreeRtosTcpStream_Destroy(stream);
    }

    void openStream() const
    {
        SolidSyslogStream_Open(stream, addr);
    }

    SolidSyslogSsize readIntoBuffer()
    {
        return SolidSyslogStream_Read(stream, readBuffer, sizeof(readBuffer));
    }
};

// clang-format on

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)
#define CHECK_SOCKET_CLOSED_ONCE()                                                                             \
    do                                                                                                         \
    {                                                                                                          \
        CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);                                                    \
        POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastClosesocketSocket()); \
    } while (0)

// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

TEST(SolidSyslogFreeRtosTcpStream, CreateReturnsNonNullStream)
{
    CHECK(stream != nullptr);
}

TEST(SolidSyslogFreeRtosTcpStream, OpenCreatesTcpSocket)
{
    openStream();
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

TEST(SolidSyslogFreeRtosTcpStream, OpenChecksIfDestinationIsInArpCache)
{
    openStream();
    CALLED_FAKE(FreeRtosArpFake_IsIpInArpCache, ONCE);
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(10, 0, 2, 2), FreeRtosArpFake_LastIsIpInArpCacheArg());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenFiresArpProbeOnCacheMiss)
{
    openStream();
    CALLED_FAKE(FreeRtosArpFake_OutputArpRequest, ONCE);
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(10, 0, 2, 2), FreeRtosArpFake_LastOutputArpRequestArg());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenYieldsAfterArpProbeOnCacheMiss)
{
    openStream();
    CALLED_FAKE(FreeRtosTaskFake_VTaskDelay, ONCE);
}

TEST(SolidSyslogFreeRtosTcpStream, OpenSkipsArpProbeAndYieldOnCacheHit)
{
    FreeRtosArpFake_SetCacheHit(true);
    openStream();
    CALLED_FAKE(FreeRtosArpFake_OutputArpRequest, NEVER);
    CALLED_FAKE(FreeRtosTaskFake_VTaskDelay, NEVER);
}

TEST(SolidSyslogFreeRtosTcpStream, OpenSetsConnectTimeoutBeforeConnect)
{
    openStream();
    LONGS_EQUAL(pdMS_TO_TICKS(200), FreeRtosSocketsFake_SndTimeoAtConnect());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenSetsRecvTimeoutBeforeConnect)
{
    openStream();
    LONGS_EQUAL(pdMS_TO_TICKS(200), FreeRtosSocketsFake_RcvTimeoAtConnect());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenCallsConnectWithSocketAndAddress)
{
    openStream();
    CALLED_FAKE(FreeRtosSocketsFake_Connect, ONCE);
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastConnectSocket());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- platform-layout cast, see setup
    POINTERS_EQUAL(reinterpret_cast<const struct freertos_sockaddr*>(addr), FreeRtosSocketsFake_LastConnectAddress());
    LONGS_EQUAL(sizeof(struct freertos_sockaddr), FreeRtosSocketsFake_LastConnectAddressLength());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenClearsSendTimeoutAfterConnect)
{
    openStream();
    LONGS_EQUAL(0, FreeRtosSocketsFake_LastSndTimeoSet());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenClearsRecvTimeoutAfterConnect)
{
    openStream();
    LONGS_EQUAL(0, FreeRtosSocketsFake_LastRcvTimeoSet());
    LONGS_EQUAL(2, FreeRtosSocketsFake_RcvTimeoSetCallCount());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenCallsSetsockoptWithReturnedSocketAndLevelZero)
{
    openStream();
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastSetsockoptSocket());
    LONGS_EQUAL(0, FreeRtosSocketsFake_LastSetsockoptLevel());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenPassesTickTypeSizedOptionLengthToSetsockopt)
{
    openStream();
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
    openStream();
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogFreeRtosTcpStream, OpenIsIdempotent)
{
    openStream();
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
    openStream();
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CALLED_FAKE(FreeRtosSocketsFake_Send, ONCE);
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastSendSocket());
    POINTERS_EQUAL(TEST_MESSAGE, FreeRtosSocketsFake_LastSendBuffer());
    LONGS_EQUAL(TEST_MESSAGE_LEN, FreeRtosSocketsFake_LastSendLength());
    LONGS_EQUAL(0, FreeRtosSocketsFake_LastSendFlags());
}

TEST(SolidSyslogFreeRtosTcpStream, SendReturnsTrueOnFullWrite)
{
    openStream();
    CHECK_TRUE(SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogFreeRtosTcpStream, SendReturnsFalseOnShortWrite)
{
    openStream();
    FreeRtosSocketsFake_SetSendReturn(TEST_SHORT_WRITE_BYTES);
    CHECK_FALSE(SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogFreeRtosTcpStream, SendDoesNotRetryAfterShortWrite)
{
    openStream();
    FreeRtosSocketsFake_SetSendReturn(TEST_SHORT_WRITE_BYTES);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CALLED_FAKE(FreeRtosSocketsFake_Send, ONCE);
}

TEST(SolidSyslogFreeRtosTcpStream, SendClosesSocketOnShortWrite)
{
    openStream();
    FreeRtosSocketsFake_SetSendReturn(TEST_SHORT_WRITE_BYTES);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogFreeRtosTcpStream, SendReturnsFalseOnError)
{
    openStream();
    FreeRtosSocketsFake_SetSendFails(true);
    CHECK_FALSE(SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogFreeRtosTcpStream, SendClosesSocketOnError)
{
    openStream();
    FreeRtosSocketsFake_SetSendFails(true);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogFreeRtosTcpStream, ReadReturnsNegativeOneBeforeOpen)
{
    LONGS_EQUAL(-1, readIntoBuffer());
    CALLED_FAKE(FreeRtosSocketsFake_Recv, NEVER);
}

TEST(SolidSyslogFreeRtosTcpStream, ReadCallsFreeRtosRecvWithSocketBufferAndLength)
{
    openStream();
    readIntoBuffer();
    CALLED_FAKE(FreeRtosSocketsFake_Recv, ONCE);
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastRecvSocket());
    POINTERS_EQUAL(readBuffer, FreeRtosSocketsFake_LastRecvBuffer());
    LONGS_EQUAL(sizeof(readBuffer), FreeRtosSocketsFake_LastRecvLength());
    LONGS_EQUAL(0, FreeRtosSocketsFake_LastRecvFlags());
}

TEST(SolidSyslogFreeRtosTcpStream, ReadReturnsBytesOnSuccess)
{
    openStream();
    FreeRtosSocketsFake_SetRecvReturn(TEST_READ_BYTES);
    LONGS_EQUAL(TEST_READ_BYTES, readIntoBuffer());
}

TEST(SolidSyslogFreeRtosTcpStream, ReadReturnsZeroWhenWouldBlock)
{
    openStream();
    FreeRtosSocketsFake_SetRecvReturn(0);
    LONGS_EQUAL(0, readIntoBuffer());
}

TEST(SolidSyslogFreeRtosTcpStream, ReadLeavesSocketOpenWhenWouldBlock)
{
    openStream();
    FreeRtosSocketsFake_SetRecvReturn(0);
    readIntoBuffer();
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, NEVER);
}

TEST(SolidSyslogFreeRtosTcpStream, ReadReturnsNegativeOneOnErrorAndClosesSocket)
{
    openStream();
    FreeRtosSocketsFake_SetRecvFails(true);
    LONGS_EQUAL(-1, readIntoBuffer());
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogFreeRtosTcpStream, CloseWithoutOpenIsNoOp)
{
    SolidSyslogStream_Close(stream);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, NEVER);
}

TEST(SolidSyslogFreeRtosTcpStream, CloseClosesOpenSocket)
{
    openStream();
    SolidSyslogStream_Close(stream);
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogFreeRtosTcpStream, CloseIsIdempotent)
{
    openStream();
    SolidSyslogStream_Close(stream);
    SolidSyslogStream_Close(stream);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
}

TEST(SolidSyslogFreeRtosTcpStream, DestroyClosesOpenSocket)
{
    openStream();
    SolidSyslogFreeRtosTcpStream_Destroy(stream);
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogFreeRtosTcpStream, DestroyAfterCloseDoesNotCloseAgain)
{
    openStream();
    SolidSyslogStream_Close(stream);
    SolidSyslogFreeRtosTcpStream_Destroy(stream);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
}
