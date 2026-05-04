#include "CppUTest/TestHarness.h"
#include "SolidSyslogAddress.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogPosixDatagram.h"
#include "SolidSyslogUdpPayload.h"
#include "SocketFake.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>

// clang-format off
static const char* const TEST_MESSAGE     = "hello";
static const size_t      TEST_MESSAGE_LEN = 5;
static const char* const TEST_ADDRESS     = "127.0.0.1";
static const int         TEST_PORT        = 514;

TEST_GROUP(SolidSyslogPosixDatagram)
{
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogDatagram* datagram = nullptr;
    SolidSyslogAddressStorage   addrStorage{};
    // cppcheck-suppress unreadVariable -- assigned in setup; cppcheck does not model CppUTest macros
    struct SolidSyslogAddress* addr = nullptr;

    void setup() override
    {
        SocketFake_Reset();
        // cppcheck-suppress unreadVariable -- used in tests; cppcheck does not model CppUTest macros
        datagram = SolidSyslogPosixDatagram_Create();
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
        SolidSyslogPosixDatagram_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogPosixDatagram, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogPosixDatagram, OpenCallsSocketOnce)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(1, SocketFake_SocketCallCount());
}

TEST(SolidSyslogPosixDatagram, OpenCallsSocketWithAF_INET)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(AF_INET, SocketFake_SocketDomain());
}

TEST(SolidSyslogPosixDatagram, OpenCallsSocketWithSOCK_DGRAM)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(SOCK_DGRAM, SocketFake_SocketType());
}

TEST(SolidSyslogPosixDatagram, OpenReturnsFalseWhenSocketFails)
{
    SocketFake_SetSocketFails(true);
    CHECK_FALSE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogPosixDatagram, OpenReturnsTrueOnSuccess)
{
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogPosixDatagram, SendToCallsSendtoOnce)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(1, SocketFake_SendtoCallCount());
}

TEST(SolidSyslogPosixDatagram, SendToPassesBuffer)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    STRCMP_EQUAL(TEST_MESSAGE, SocketFake_LastBufAsString());
}

TEST(SolidSyslogPosixDatagram, SendToPassesLength)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(TEST_MESSAGE_LEN, SocketFake_LastLen());
}

TEST(SolidSyslogPosixDatagram, SendToPassesFlagsZero)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(0, SocketFake_LastFlags());
}

TEST(SolidSyslogPosixDatagram, SendToPassesSocketFd)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastSendtoFd());
}

TEST(SolidSyslogPosixDatagram, SendToPassesAddressPort)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(TEST_PORT, SocketFake_LastPort());
}

TEST(SolidSyslogPosixDatagram, SendToPassesAddressHost)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    STRCMP_EQUAL(TEST_ADDRESS, SocketFake_LastAddrAsString());
}

TEST(SolidSyslogPosixDatagram, SendToPassesAddrlenOfSockaddrIn)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(sizeof(struct sockaddr_in), SocketFake_LastAddrLen());
}

TEST(SolidSyslogPosixDatagram, SendToReturnsSentOnSuccess)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SENT, SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr));
}

TEST(SolidSyslogPosixDatagram, SendToReturnsFailedOnSendtoFailure)
{
    SolidSyslogDatagram_Open(datagram);
    SocketFake_SetSendtoFails(true);
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_FAILED, SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr));
}

TEST(SolidSyslogPosixDatagram, CloseCallsCloseOnce)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    LONGS_EQUAL(1, SocketFake_CloseCallCount());
}

TEST(SolidSyslogPosixDatagram, CloseCalledWithSocketFd)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastClosedFd());
}

TEST(SolidSyslogPosixDatagram, MaxPayloadFallsBackToIpv6SafePayload)
{
    LONGS_EQUAL(SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogPosixDatagram, OpenDoesNotConnect)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(0, SocketFake_ConnectCallCount());
}

TEST(SolidSyslogPosixDatagram, SendToConnectsOnFirstCall)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(1, SocketFake_ConnectCallCount());
}

TEST(SolidSyslogPosixDatagram, SendToConnectsOnceAcrossMultipleCalls)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(1, SocketFake_ConnectCallCount());
}

TEST(SolidSyslogPosixDatagram, FirstSendEnablesPmtuDiscovery)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    CHECK_TRUE(SocketFake_HasSetSockOpt(IPPROTO_IP, IP_MTU_DISCOVER));
}

TEST(SolidSyslogPosixDatagram, SendToReturnsOversizeOnEmsgsize)
{
    SolidSyslogDatagram_Open(datagram);
    SocketFake_FailNextSendtoWithErrno(EMSGSIZE);
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_OVERSIZE, SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr));
}

TEST(SolidSyslogPosixDatagram, MaxPayloadAfterConnectQueriesIpMtu)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    SolidSyslogDatagram_MaxPayload(datagram);
    LONGS_EQUAL(1, SocketFake_GetSockOptCallCount());
}

TEST(SolidSyslogPosixDatagram, MaxPayloadConvertsIpMtuViaFromMtu)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    SocketFake_SetIpMtu(1500);
    LONGS_EQUAL(1472, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogPosixDatagram, MaxPayloadFallsBackWhenIpMtuLookupFails)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    SocketFake_SetIpMtuLookupFails(true);
    LONGS_EQUAL(SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogPosixDatagram, SendToReturnsFailedWhenConnectFails)
{
    SolidSyslogDatagram_Open(datagram);
    SocketFake_SetConnectFails(true);
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_FAILED, SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr));
}
