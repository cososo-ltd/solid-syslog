#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstdint>

#include "SolidSyslogAddress.h"
#include "SolidSyslogGetAddrInfoResolver.h"
#include "SolidSyslogResolver.h"
#include "SocketFake.h"
#include "SolidSyslogTransport.h"
#include "CppUTest/TestHarness.h"

// clang-format off
static const char* const TEST_HOST           = "127.0.0.1";
static const uint16_t    TEST_PORT           = 514;
static const char* const TEST_ALTERNATE_HOST = "192.168.1.1";
static const uint16_t    TEST_ALTERNATE_PORT = 9999;
// clang-format on

// clang-format off
TEST_GROUP(SolidSyslogGetAddrInfoResolver)
{
    struct SolidSyslogResolver* resolver = nullptr;
    SolidSyslogAddressStorage   resultStorage{};

    void setup() override
    {
        SocketFake_Reset();
        resolver = SolidSyslogGetAddrInfoResolver_Create();
    }

    void teardown() override
    {
        SolidSyslogGetAddrInfoResolver_Destroy();
    }

    bool Resolve(const char* host, uint16_t port, enum SolidSyslogTransport transport = SOLIDSYSLOG_TRANSPORT_UDP)
    {
        struct SolidSyslogAddress* address = SolidSyslogAddress_FromStorage(&resultStorage);
        return SolidSyslogResolver_Resolve(resolver, transport, host, port, address);
    }

    // NOLINTNEXTLINE(modernize-use-nodiscard) -- used through accessor syntax in tests
    const struct sockaddr_in* Result() const
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- char-type aliasing, legal and necessary
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(&resultStorage);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- reinterpret to platform layout, storage is intptr_t-aligned
        return reinterpret_cast<const struct sockaddr_in*>(bytes);
    }
};

// clang-format on

TEST(SolidSyslogGetAddrInfoResolver, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogGetAddrInfoResolver, ReturnsTrueOnSuccess)
{
    CHECK_TRUE(Resolve(TEST_HOST, TEST_PORT));
}

TEST(SolidSyslogGetAddrInfoResolver, PopulatesAddressFamily)
{
    Resolve(TEST_HOST, TEST_PORT);
    LONGS_EQUAL(AF_INET, Result()->sin_family);
}

TEST(SolidSyslogGetAddrInfoResolver, PopulatesResolvedAddressFromHostArgument)
{
    Resolve(TEST_HOST, TEST_PORT);
    char addrString[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &Result()->sin_addr, addrString, sizeof(addrString));
    STRCMP_EQUAL(TEST_HOST, addrString);
}

TEST(SolidSyslogGetAddrInfoResolver, PopulatesPortFromPortArgument)
{
    Resolve(TEST_HOST, TEST_ALTERNATE_PORT);
    LONGS_EQUAL(TEST_ALTERNATE_PORT, ntohs(Result()->sin_port));
}

TEST(SolidSyslogGetAddrInfoResolver, GetAddrInfoCalledWithHostArgument)
{
    Resolve(TEST_ALTERNATE_HOST, TEST_PORT);
    LONGS_EQUAL(1, SocketFake_GetAddrInfoCallCount());
    STRCMP_EQUAL(TEST_ALTERNATE_HOST, SocketFake_LastGetAddrInfoHostname());
}

TEST(SolidSyslogGetAddrInfoResolver, UdpTransportPassesDatagramSocktype)
{
    Resolve(TEST_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_UDP);
    LONGS_EQUAL(SOCK_DGRAM, SocketFake_LastGetAddrInfoSocktype());
}

TEST(SolidSyslogGetAddrInfoResolver, TcpTransportPassesStreamSocktype)
{
    Resolve(TEST_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_TCP);
    LONGS_EQUAL(SOCK_STREAM, SocketFake_LastGetAddrInfoSocktype());
}

TEST(SolidSyslogGetAddrInfoResolver, ReturnsFalseWhenGetAddrInfoFails)
{
    SocketFake_SetGetAddrInfoFails(true);
    CHECK_FALSE(Resolve(TEST_HOST, TEST_PORT));
}

TEST(SolidSyslogGetAddrInfoResolver, DoesNotFreeAddrInfoWhenGetAddrInfoFails)
{
    SocketFake_SetGetAddrInfoFails(true);
    Resolve(TEST_HOST, TEST_PORT);
    LONGS_EQUAL(0, SocketFake_FreeAddrInfoCallCount());
}

TEST(SolidSyslogGetAddrInfoResolver, FreesAddrInfoOnSuccess)
{
    Resolve(TEST_HOST, TEST_PORT);
    LONGS_EQUAL(1, SocketFake_FreeAddrInfoCallCount());
}
