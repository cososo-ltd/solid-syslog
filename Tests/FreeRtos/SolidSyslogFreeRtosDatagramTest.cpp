#include "CppUTest/TestHarness.h"

#include "SolidSyslogAddress.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogFreeRtosDatagram.h"
#include "SolidSyslogUdpPayload.h"

#include "FreeRtosArpFake.h"
#include "FreeRtosSocketsFake.h"
#include "FreeRtosTaskFake.h"

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

static const uint16_t TEST_PORT = 514;

TEST_GROUP(SolidSyslogFreeRtosDatagram)
{
    SolidSyslogFreeRtosDatagramStorage storage{};
    struct SolidSyslogDatagram*        datagram = nullptr;
    SolidSyslogAddressStorage          addrStorage{};
    struct SolidSyslogAddress*         addr = nullptr;

    void setup() override
    {
        FreeRtosSocketsFake_Reset();
        FreeRtosArpFake_Reset();
        FreeRtosTaskFake_Reset();
        datagram = SolidSyslogFreeRtosDatagram_Create(&storage);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- char-type aliasing into platform layout, storage is intptr_t-aligned
        auto* sin                  = reinterpret_cast<struct freertos_sockaddr*>(&addrStorage);
        sin->sin_family            = FREERTOS_AF_INET;
        sin->sin_port              = FreeRTOS_htons(TEST_PORT);
        sin->sin_address.ulIP_IPv4 = FreeRTOS_inet_addr_quick(127, 0, 0, 1);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- platform-layout cast, see above
        addr = reinterpret_cast<struct SolidSyslogAddress*>(&addrStorage);
    }

    void openAndSendOnce()
    {
        SolidSyslogDatagram_Open(datagram);
        SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);
    }
};

TEST(SolidSyslogFreeRtosDatagram, CreateReturnsNonNullDatagram)
{
    CHECK(datagram != nullptr);
}

TEST(SolidSyslogFreeRtosDatagram, OpenCreatesUdpSocket)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(1, FreeRtosSocketsFake_SocketCallCount());
    LONGS_EQUAL(FREERTOS_AF_INET, FreeRtosSocketsFake_LastSocketDomain());
    LONGS_EQUAL(FREERTOS_SOCK_DGRAM, FreeRtosSocketsFake_LastSocketType());
    LONGS_EQUAL(FREERTOS_IPPROTO_UDP, FreeRtosSocketsFake_LastSocketProtocol());
}

TEST(SolidSyslogFreeRtosDatagram, OpenReturnsTrueOnSuccess)
{
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogFreeRtosDatagram, OpenReturnsFalseWhenSocketFails)
{
    FreeRtosSocketsFake_SetSocketFails(true);
    CHECK_FALSE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogFreeRtosDatagram, OpenIsIdempotent)
{
    SolidSyslogDatagram_Open(datagram);
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));
    LONGS_EQUAL(1, FreeRtosSocketsFake_SocketCallCount());
}

TEST(SolidSyslogFreeRtosDatagram, MaxPayloadReturnsIpv6SafeDefault)
{
    LONGS_EQUAL(SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogFreeRtosDatagram, SendToFailsBeforeOpen)
{
    enum SolidSyslogDatagramSendResult result = SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_FAILED, result);
    LONGS_EQUAL(0, FreeRtosSocketsFake_SendtoCallCount());
}

TEST(SolidSyslogFreeRtosDatagram, SendToFailsWhenSendtoErrors)
{
    SolidSyslogDatagram_Open(datagram);
    FreeRtosSocketsFake_SetSendtoFails(true);
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_FAILED, SolidSyslogDatagram_SendTo(datagram, "x", 1, addr));
}

TEST(SolidSyslogFreeRtosDatagram, CloseClosesSocket)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    LONGS_EQUAL(1, FreeRtosSocketsFake_ClosesocketCallCount());
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastClosesocketSocket());
}

TEST(SolidSyslogFreeRtosDatagram, SendToFailsAfterClose)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    enum SolidSyslogDatagramSendResult result = SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_FAILED, result);
    LONGS_EQUAL(0, FreeRtosSocketsFake_SendtoCallCount());
}

TEST(SolidSyslogFreeRtosDatagram, DestroyClosesOpenSocket)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogFreeRtosDatagram_Destroy(datagram);
    LONGS_EQUAL(1, FreeRtosSocketsFake_ClosesocketCallCount());
}

TEST(SolidSyslogFreeRtosDatagram, CloseIsIdempotent)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    SolidSyslogDatagram_Close(datagram);
    LONGS_EQUAL(1, FreeRtosSocketsFake_ClosesocketCallCount());
}

TEST(SolidSyslogFreeRtosDatagram, CloseWithoutOpenIsNoOp)
{
    SolidSyslogDatagram_Close(datagram);
    LONGS_EQUAL(0, FreeRtosSocketsFake_ClosesocketCallCount());
}

TEST(SolidSyslogFreeRtosDatagram, DestroyAfterCloseDoesNotCloseAgain)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    SolidSyslogFreeRtosDatagram_Destroy(datagram);
    LONGS_EQUAL(1, FreeRtosSocketsFake_ClosesocketCallCount());
}

TEST(SolidSyslogFreeRtosDatagram, SendToSendsBufferToDestinationAfterOpen)
{
    static const char   TEST_MESSAGE[]   = "hello";
    static const size_t TEST_MESSAGE_LEN = sizeof(TEST_MESSAGE) - 1;

    SolidSyslogDatagram_Open(datagram);
    enum SolidSyslogDatagramSendResult result = SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SENT, result);
    LONGS_EQUAL(1, FreeRtosSocketsFake_SendtoCallCount());
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastSendtoSocket());
    POINTERS_EQUAL(TEST_MESSAGE, FreeRtosSocketsFake_LastSendtoBuffer());
    LONGS_EQUAL(TEST_MESSAGE_LEN, FreeRtosSocketsFake_LastSendtoLength());
    LONGS_EQUAL(0, FreeRtosSocketsFake_LastSendtoFlags());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- platform-layout cast, see setup
    POINTERS_EQUAL(reinterpret_cast<const struct freertos_sockaddr*>(addr), FreeRtosSocketsFake_LastSendtoDestination());
    LONGS_EQUAL(sizeof(struct freertos_sockaddr), FreeRtosSocketsFake_LastSendtoDestinationLength());
}

TEST(SolidSyslogFreeRtosDatagram, SendToChecksIfIpIsInArpCache)
{
    openAndSendOnce();

    LONGS_EQUAL(1, FreeRtosArpFake_IsIpInArpCacheCallCount());
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(127, 0, 0, 1), FreeRtosArpFake_LastIsIpInArpCacheArg());
}

TEST(SolidSyslogFreeRtosDatagram, SendToFiresArpProbeOnCacheMiss)
{
    openAndSendOnce();

    LONGS_EQUAL(1, FreeRtosArpFake_OutputArpRequestCallCount());
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(127, 0, 0, 1), FreeRtosArpFake_LastOutputArpRequestArg());
}

TEST(SolidSyslogFreeRtosDatagram, SendToYieldsAfterArpProbeOnCacheMiss)
{
    openAndSendOnce();

    LONGS_EQUAL(1, FreeRtosTaskFake_VTaskDelayCallCount());
}

TEST(SolidSyslogFreeRtosDatagram, SendToSkipsArpProbeAndYieldOnCacheHit)
{
    SolidSyslogDatagram_Open(datagram);
    FreeRtosArpFake_SetCacheHit(true);

    SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);

    LONGS_EQUAL(0, FreeRtosArpFake_OutputArpRequestCallCount());
    LONGS_EQUAL(0, FreeRtosTaskFake_VTaskDelayCallCount());
    LONGS_EQUAL(1, FreeRtosSocketsFake_SendtoCallCount());
}

TEST(SolidSyslogFreeRtosDatagram, SendToReChecksArpCacheOnEachCall)
{
    SolidSyslogDatagram_Open(datagram);

    FreeRtosArpFake_SetCacheHit(true);
    SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);
    LONGS_EQUAL(0, FreeRtosArpFake_OutputArpRequestCallCount());

    FreeRtosArpFake_SetCacheHit(false);
    SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);
    LONGS_EQUAL(1, FreeRtosArpFake_OutputArpRequestCallCount());

    FreeRtosArpFake_SetCacheHit(true);
    SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);
    LONGS_EQUAL(1, FreeRtosArpFake_OutputArpRequestCallCount());
}

TEST(SolidSyslogFreeRtosDatagram, SendToForwardsLengthVerbatim)
{
    static const char TEST_BUFFER[1024] = {};
    SolidSyslogDatagram_Open(datagram);

    SolidSyslogDatagram_SendTo(datagram, TEST_BUFFER, 1, addr);
    LONGS_EQUAL(1, FreeRtosSocketsFake_LastSendtoLength());

    SolidSyslogDatagram_SendTo(datagram, TEST_BUFFER, 1232, addr);
    LONGS_EQUAL(1232, FreeRtosSocketsFake_LastSendtoLength());
}
