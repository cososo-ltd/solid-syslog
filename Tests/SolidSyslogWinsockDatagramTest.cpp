#include "CppUTest/TestHarness.h"
#include "SolidSyslogAddress.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogUdpPayload.h"
#include "SolidSyslogWinsockDatagram.h"
#include "SolidSyslogWinsockDatagramInternal.h"
#include "WinsockFake.h"
#include <cstdint>
#include <winsock2.h>
#include <ws2tcpip.h>

// clang-format off
static const char* const TEST_MESSAGE     = "hello";
static const size_t      TEST_MESSAGE_LEN = 5;
static const char* const TEST_ADDRESS     = "127.0.0.1";
static const int         TEST_PORT        = 514;

TEST_GROUP(SolidSyslogWinsockDatagram)
{
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogDatagram* datagram = nullptr;
    SolidSyslogAddressStorage   addrStorage{};
    // cppcheck-suppress unreadVariable -- assigned in setup; cppcheck does not model CppUTest macros
    struct SolidSyslogAddress* addr = nullptr;

    void setup() override
    {
        WinsockFake_Reset();
        UT_PTR_SET(Winsock_socket, WinsockFake_socket);
        UT_PTR_SET(Winsock_sendto, WinsockFake_sendto);
        UT_PTR_SET(Winsock_closesocket, WinsockFake_closesocket);
        UT_PTR_SET(Winsock_connect, WinsockFake_connect);
        UT_PTR_SET(Winsock_setsockopt, WinsockFake_setsockopt);
        UT_PTR_SET(Winsock_getsockopt, WinsockFake_getsockopt);
        // cppcheck-suppress unreadVariable -- used in tests; cppcheck does not model CppUTest macros
        datagram = SolidSyslogWinsockDatagram_Create();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- char-type aliasing, legal and necessary
        auto* bytes = reinterpret_cast<std::uint8_t*>(&addrStorage);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- reinterpret to platform layout, storage is intptr_t-aligned
        auto* sin       = reinterpret_cast<struct sockaddr_in*>(bytes);
        sin->sin_family = AF_INET;
        sin->sin_port   = htons(TEST_PORT);
        inet_pton(AF_INET, TEST_ADDRESS, &sin->sin_addr);
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        addr = SolidSyslogAddress_FromStorage(&addrStorage);
    }

    void teardown() override
    {
        SolidSyslogWinsockDatagram_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogWinsockDatagram, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogWinsockDatagram, OpenCallsSocketOnce)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(1, WinsockFake_SocketCallCount());
}

TEST(SolidSyslogWinsockDatagram, OpenCallsSocketWithAF_INET)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(AF_INET, WinsockFake_SocketDomain());
}

TEST(SolidSyslogWinsockDatagram, OpenCallsSocketWithSOCK_DGRAM)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(SOCK_DGRAM, WinsockFake_SocketType());
}

TEST(SolidSyslogWinsockDatagram, OpenReturnsFalseWhenSocketFails)
{
    WinsockFake_SetSocketFails(true);
    CHECK_FALSE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogWinsockDatagram, OpenReturnsTrueOnSuccess)
{
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogWinsockDatagram, SendToCallsSendtoOnce)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(1, WinsockFake_SendtoCallCount());
}

TEST(SolidSyslogWinsockDatagram, SendToPassesBuffer)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    STRCMP_EQUAL(TEST_MESSAGE, WinsockFake_LastBufAsString());
}

TEST(SolidSyslogWinsockDatagram, SendToPassesLength)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(TEST_MESSAGE_LEN, WinsockFake_LastLen());
}

TEST(SolidSyslogWinsockDatagram, SendToPassesFlagsZero)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(0, WinsockFake_LastFlags());
}

TEST(SolidSyslogWinsockDatagram, SendToPassesSocketFd)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    CHECK(WinsockFake_SocketFd() == WinsockFake_LastSendtoFd());
}

TEST(SolidSyslogWinsockDatagram, SendToPassesAddressPort)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(TEST_PORT, WinsockFake_LastPort());
}

TEST(SolidSyslogWinsockDatagram, SendToPassesAddressHost)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    STRCMP_EQUAL(TEST_ADDRESS, WinsockFake_LastAddrAsString());
}

TEST(SolidSyslogWinsockDatagram, SendToPassesAddrlenOfSockaddrIn)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL((int) sizeof(struct sockaddr_in), WinsockFake_LastAddrLen());
}

TEST(SolidSyslogWinsockDatagram, SendToReturnsSentOnSuccess)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SENT, SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr));
}

TEST(SolidSyslogWinsockDatagram, SendToReturnsFailedOnSendtoFailure)
{
    SolidSyslogDatagram_Open(datagram);
    WinsockFake_SetSendtoFails(true);
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_FAILED, SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr));
}

TEST(SolidSyslogWinsockDatagram, CloseCallsCloseOnce)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    LONGS_EQUAL(1, WinsockFake_CloseCallCount());
}

TEST(SolidSyslogWinsockDatagram, CloseCalledWithSocketFd)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    CHECK(WinsockFake_SocketFd() == WinsockFake_LastClosedFd());
}

TEST(SolidSyslogWinsockDatagram, MaxPayloadFallsBackToIpv6SafePayload)
{
    LONGS_EQUAL(SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogWinsockDatagram, OpenDoesNotConnect)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(0, WinsockFake_ConnectCallCount());
}

TEST(SolidSyslogWinsockDatagram, SendToConnectsOnFirstCall)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(1, WinsockFake_ConnectCallCount());
}

TEST(SolidSyslogWinsockDatagram, SendToConnectsOnceAcrossMultipleCalls)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(1, WinsockFake_ConnectCallCount());
}

TEST(SolidSyslogWinsockDatagram, FirstSendEnablesPmtuDiscovery)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    CHECK_TRUE(WinsockFake_HasSetSockOpt(IPPROTO_IP, IP_MTU_DISCOVER));
}

TEST(SolidSyslogWinsockDatagram, SendToReturnsOversizeOnWsaemsgsize)
{
    SolidSyslogDatagram_Open(datagram);
    WinsockFake_FailNextSendtoWithLastError(WSAEMSGSIZE);
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_OVERSIZE, SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr));
}

TEST(SolidSyslogWinsockDatagram, SendToReturnsFailedWhenConnectFails)
{
    SolidSyslogDatagram_Open(datagram);
    WinsockFake_SetConnectFails(true);
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_FAILED, SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr));
}

TEST(SolidSyslogWinsockDatagram, MaxPayloadAfterConnectQueriesIpMtu)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    SolidSyslogDatagram_MaxPayload(datagram);
    LONGS_EQUAL(1, WinsockFake_GetSockOptCallCount());
}

TEST(SolidSyslogWinsockDatagram, MaxPayloadConvertsIpMtuViaFromMtu)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    WinsockFake_SetIpMtu(1500);
    LONGS_EQUAL(1472, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogWinsockDatagram, MaxPayloadFallsBackWhenIpMtuLookupFails)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    WinsockFake_SetIpMtuLookupFails(true);
    LONGS_EQUAL(SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD, SolidSyslogDatagram_MaxPayload(datagram));
}
