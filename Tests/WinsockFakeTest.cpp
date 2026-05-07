#include "CppUTest/TestHarness.h"
#include "WinsockFake.h"
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>

// clang-format off
static const char* const TEST_MESSAGE     = "hello";
static const int         TEST_MESSAGE_LEN = 5;
static const char* const TEST_HOST        = "127.0.0.1";
static const int         TEST_PORT        = 514;

TEST_GROUP(WinsockFake)
{
    struct sockaddr_in destination{};

    void setup() override
    {
        WinsockFake_Reset();
        destination.sin_family = AF_INET;
        destination.sin_port   = htons(TEST_PORT);
        inet_pton(AF_INET, TEST_HOST, &destination.sin_addr);
    }
};

// clang-format on

TEST(WinsockFake, SocketRecordsCall)
{
    WinsockFake_socket(AF_INET, SOCK_DGRAM, 0);
    LONGS_EQUAL(1, WinsockFake_SocketCallCount());
    LONGS_EQUAL(AF_INET, WinsockFake_SocketDomain());
    LONGS_EQUAL(SOCK_DGRAM, WinsockFake_SocketType());
}

TEST(WinsockFake, SocketReturnsInvalidSocketWhenFailing)
{
    WinsockFake_SetSocketFails(true);
    SOCKET fd = WinsockFake_socket(AF_INET, SOCK_DGRAM, 0);
    CHECK(fd == INVALID_SOCKET);
}

TEST(WinsockFake, SocketReturnsValidHandleOnSuccess)
{
    SOCKET fd = WinsockFake_socket(AF_INET, SOCK_DGRAM, 0);
    CHECK(fd != INVALID_SOCKET);
}

TEST(WinsockFake, SendtoRecordsBufferAndAddress)
{
    SOCKET fd = WinsockFake_socket(AF_INET, SOCK_DGRAM, 0);
    WinsockFake_sendto(fd, TEST_MESSAGE, TEST_MESSAGE_LEN, 0, (const struct sockaddr*) &destination, sizeof(destination));
    LONGS_EQUAL(1, WinsockFake_SendtoCallCount());
    STRCMP_EQUAL(TEST_MESSAGE, WinsockFake_LastBufAsString());
    LONGS_EQUAL(TEST_MESSAGE_LEN, (int) WinsockFake_LastLen());
    LONGS_EQUAL(TEST_PORT, WinsockFake_LastPort());
    STRCMP_EQUAL(TEST_HOST, WinsockFake_LastAddrAsString());
    LONGS_EQUAL((int) sizeof(destination), WinsockFake_LastAddrLen());
}

TEST(WinsockFake, SendtoReturnsSocketErrorWhenFailing)
{
    WinsockFake_SetSendtoFails(true);
    int result = WinsockFake_sendto(INVALID_SOCKET, TEST_MESSAGE, TEST_MESSAGE_LEN, 0, (const struct sockaddr*) &destination, sizeof(destination));
    LONGS_EQUAL(SOCKET_ERROR, result);
}

TEST(WinsockFake, SendtoReturnsLengthOnSuccess)
{
    int result = WinsockFake_sendto(INVALID_SOCKET, TEST_MESSAGE, TEST_MESSAGE_LEN, 0, (const struct sockaddr*) &destination, sizeof(destination));
    LONGS_EQUAL(TEST_MESSAGE_LEN, result);
}

TEST(WinsockFake, ClosesocketRecordsCall)
{
    SOCKET fd = WinsockFake_socket(AF_INET, SOCK_DGRAM, 0);
    WinsockFake_closesocket(fd);
    LONGS_EQUAL(1, WinsockFake_CloseCallCount());
    CHECK(WinsockFake_LastClosedFd() == fd);
}

TEST(WinsockFake, GetAddrInfoRecordsHostnameAndSocktype)
{
    struct addrinfo hints = {0, 0, 0, 0, 0, nullptr, nullptr, nullptr};
    hints.ai_socktype     = SOCK_DGRAM;
    struct addrinfo* res  = nullptr;
    WinsockFake_getaddrinfo(TEST_HOST, nullptr, &hints, &res);
    LONGS_EQUAL(1, WinsockFake_GetAddrInfoCallCount());
    STRCMP_EQUAL(TEST_HOST, WinsockFake_LastGetAddrInfoHostname());
    LONGS_EQUAL(SOCK_DGRAM, WinsockFake_LastGetAddrInfoSocktype());
    CHECK(res != nullptr);
}

TEST(WinsockFake, GetAddrInfoReturnsFailureCode)
{
    WinsockFake_SetGetAddrInfoFails(true);
    struct addrinfo* res = nullptr;
    int              rc  = WinsockFake_getaddrinfo(TEST_HOST, nullptr, nullptr, &res);
    LONGS_EQUAL(EAI_FAIL, rc);
}

TEST(WinsockFake, FreeAddrInfoRecordsCall)
{
    struct addrinfo* res = nullptr;
    WinsockFake_getaddrinfo(TEST_HOST, nullptr, nullptr, &res);
    WinsockFake_freeaddrinfo(res);
    LONGS_EQUAL(1, WinsockFake_FreeAddrInfoCallCount());
}

TEST(WinsockFake, ConnectRecordsCallAndAddress)
{
    SOCKET fd = WinsockFake_socket(AF_INET, SOCK_STREAM, 0);
    WinsockFake_connect(fd, (const struct sockaddr*) &destination, sizeof(destination));
    LONGS_EQUAL(1, WinsockFake_ConnectCallCount());
    CHECK(WinsockFake_LastConnectFd() == fd);
    LONGS_EQUAL(TEST_PORT, WinsockFake_LastConnectPort());
    STRCMP_EQUAL(TEST_HOST, WinsockFake_LastConnectAddrAsString());
}

TEST(WinsockFake, ConnectReturnsZeroOnSuccess)
{
    int rc = WinsockFake_connect(INVALID_SOCKET, (const struct sockaddr*) &destination, sizeof(destination));
    LONGS_EQUAL(0, rc);
}

TEST(WinsockFake, ConnectReturnsSocketErrorWhenFailing)
{
    WinsockFake_SetConnectFails(true);
    int rc = WinsockFake_connect(INVALID_SOCKET, (const struct sockaddr*) &destination, sizeof(destination));
    LONGS_EQUAL(SOCKET_ERROR, rc);
}

TEST(WinsockFake, SendRecordsBufferAndFlags)
{
    SOCKET fd = WinsockFake_socket(AF_INET, SOCK_STREAM, 0);
    WinsockFake_send(fd, TEST_MESSAGE, TEST_MESSAGE_LEN, 0);
    LONGS_EQUAL(1, WinsockFake_SendCallCount());
    STRCMP_EQUAL(TEST_MESSAGE, WinsockFake_SendBufAsString(0));
    LONGS_EQUAL(TEST_MESSAGE_LEN, (int) WinsockFake_SendLen(0));
    LONGS_EQUAL(0, WinsockFake_SendFlags(0));
    CHECK(WinsockFake_LastSendFd() == fd);
}

TEST(WinsockFake, SendReturnsLengthOnSuccess)
{
    int rc = WinsockFake_send(INVALID_SOCKET, TEST_MESSAGE, TEST_MESSAGE_LEN, 0);
    LONGS_EQUAL(TEST_MESSAGE_LEN, rc);
}

TEST(WinsockFake, SendReturnsSocketErrorWhenFailing)
{
    WinsockFake_SetSendFails(true);
    int rc = WinsockFake_send(INVALID_SOCKET, TEST_MESSAGE, TEST_MESSAGE_LEN, 0);
    LONGS_EQUAL(SOCKET_ERROR, rc);
}

TEST(WinsockFake, SendReturnsConfiguredValueWhenOverridden)
{
    WinsockFake_SetSendReturn(3);
    int rc = WinsockFake_send(INVALID_SOCKET, TEST_MESSAGE, TEST_MESSAGE_LEN, 0);
    LONGS_EQUAL(3, rc);
}

TEST(WinsockFake, SendAccessorsReturnEmptyForOutOfRangeIndex)
{
    STRCMP_EQUAL("", WinsockFake_SendBufAsString(-1));
    STRCMP_EQUAL("", WinsockFake_SendBufAsString(99));
    LONGS_EQUAL(0, (int) WinsockFake_SendLen(-1));
    LONGS_EQUAL(0, (int) WinsockFake_SendLen(99));
    LONGS_EQUAL(0, WinsockFake_SendFlags(-1));
    LONGS_EQUAL(0, WinsockFake_SendFlags(99));
}

TEST(WinsockFake, RecvRecordsCall)
{
    char   buf[16];
    SOCKET fd = WinsockFake_socket(AF_INET, SOCK_STREAM, 0);
    WinsockFake_recv(fd, buf, sizeof(buf), 0);
    LONGS_EQUAL(1, WinsockFake_RecvCallCount());
    CHECK(WinsockFake_LastRecvFd() == fd);
    POINTERS_EQUAL(buf, WinsockFake_LastRecvBuf());
    LONGS_EQUAL(sizeof(buf), WinsockFake_LastRecvLen());
    LONGS_EQUAL(0, WinsockFake_LastRecvFlags());
}

TEST(WinsockFake, RecvReturnsConfiguredValue)
{
    char buf[16];
    WinsockFake_SetRecvReturn(7);
    int n = WinsockFake_recv(INVALID_SOCKET, buf, sizeof(buf), 0);
    LONGS_EQUAL(7, n);
}

TEST(WinsockFake, SetSockOptRecordsLevelAndOptname)
{
    int enable = 1;
    WinsockFake_setsockopt(INVALID_SOCKET, IPPROTO_TCP, TCP_NODELAY, (const char*) &enable, sizeof(enable));
    LONGS_EQUAL(1, WinsockFake_SetSockOptCallCount());
    LONGS_EQUAL(IPPROTO_TCP, WinsockFake_LastSetSockOptLevel());
    LONGS_EQUAL(TCP_NODELAY, WinsockFake_LastSetSockOptOptname());
}

TEST(WinsockFake, HasSetSockOptFindsRecordedPair)
{
    int enable = 1;
    WinsockFake_setsockopt(INVALID_SOCKET, IPPROTO_TCP, TCP_NODELAY, (const char*) &enable, sizeof(enable));
    CHECK_TRUE(WinsockFake_HasSetSockOpt(IPPROTO_TCP, TCP_NODELAY));
}

TEST(WinsockFake, HasSetSockOptReturnsFalseForUnseenPair)
{
    CHECK_FALSE(WinsockFake_HasSetSockOpt(IPPROTO_TCP, TCP_NODELAY));
}

TEST(WinsockFake, LastSetSockOptValueReturnsCapturedIntForRecordedPair)
{
    int idleSeconds = 45;
    WinsockFake_setsockopt(INVALID_SOCKET, IPPROTO_TCP, TCP_KEEPIDLE, (const char*) &idleSeconds, sizeof(idleSeconds));
    LONGS_EQUAL(45, WinsockFake_LastSetSockOptValue(IPPROTO_TCP, TCP_KEEPIDLE));
}

TEST(WinsockFake, LastSetSockOptValueReturnsZeroForUnseenPair)
{
    LONGS_EQUAL(0, WinsockFake_LastSetSockOptValue(IPPROTO_TCP, TCP_KEEPIDLE));
}

TEST(WinsockFake, ResetClearsCounters)
{
    WinsockFake_socket(AF_INET, SOCK_DGRAM, 0);
    WinsockFake_Reset();
    LONGS_EQUAL(0, WinsockFake_SocketCallCount());
    LONGS_EQUAL(0, WinsockFake_SendtoCallCount());
    LONGS_EQUAL(0, WinsockFake_ConnectCallCount());
    LONGS_EQUAL(0, WinsockFake_SendCallCount());
    LONGS_EQUAL(0, WinsockFake_RecvCallCount());
    LONGS_EQUAL(0, WinsockFake_SetSockOptCallCount());
    LONGS_EQUAL(0, WinsockFake_CloseCallCount());
    LONGS_EQUAL(0, WinsockFake_GetAddrInfoCallCount());
    LONGS_EQUAL(0, WinsockFake_FreeAddrInfoCallCount());
}
