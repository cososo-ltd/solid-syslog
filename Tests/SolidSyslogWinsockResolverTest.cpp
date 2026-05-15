#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros
#include "SolidSyslogAddress.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogWinsockResolver.h"
#include "SolidSyslogWinsockResolverInternal.h"
#include "WinsockFake.h"
#include <cstdint>
#include <winsock2.h>
#include <ws2tcpip.h>

// clang-format off
static const char* const TEST_HOST           = "127.0.0.1";
static const uint16_t    TEST_PORT           = 514;
static const char* const TEST_ALTERNATE_HOST = "192.168.1.1";
static const uint16_t    TEST_ALTERNATE_PORT = 9999;
// clang-format on

// clang-format off
TEST_GROUP(SolidSyslogWinsockResolver)
{
    struct SolidSyslogResolver* resolver = nullptr;
    SolidSyslogAddressStorage   resultStorage{};

    void setup() override
    {
        WinsockFake_Reset();
        UT_PTR_SET(Winsock_getaddrinfo, WinsockFake_getaddrinfo);
        UT_PTR_SET(Winsock_freeaddrinfo, WinsockFake_freeaddrinfo);
        resolver = SolidSyslogWinsockResolver_Create();
    }

    void teardown() override
    {
        SolidSyslogWinsockResolver_Destroy();
    }

    bool Resolve(const char* host, uint16_t port, enum SolidSyslogTransport transport = SolidSyslogTransport_Udp)
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

TEST(SolidSyslogWinsockResolver, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogWinsockResolver, ReturnsTrueOnSuccess)
{
    CHECK_TRUE(Resolve(TEST_HOST, TEST_PORT));
}

TEST(SolidSyslogWinsockResolver, PopulatesAddressFamily)
{
    Resolve(TEST_HOST, TEST_PORT);
    LONGS_EQUAL(AF_INET, Result()->sin_family);
}

TEST(SolidSyslogWinsockResolver, PopulatesResolvedAddressFromHostArgument)
{
    Resolve(TEST_HOST, TEST_PORT);
    char addrString[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &Result()->sin_addr, addrString, sizeof(addrString));
    STRCMP_EQUAL(TEST_HOST, addrString);
}

TEST(SolidSyslogWinsockResolver, PopulatesPortFromPortArgument)
{
    Resolve(TEST_HOST, TEST_ALTERNATE_PORT);
    LONGS_EQUAL(TEST_ALTERNATE_PORT, ntohs(Result()->sin_port));
}

TEST(SolidSyslogWinsockResolver, GetAddrInfoCalledWithHostArgument)
{
    Resolve(TEST_ALTERNATE_HOST, TEST_PORT);
    CALLED_FAKE(WinsockFake_GetAddrInfo, ONCE);
    STRCMP_EQUAL(TEST_ALTERNATE_HOST, WinsockFake_LastGetAddrInfoHostname());
}

TEST(SolidSyslogWinsockResolver, UdpTransportPassesDatagramSocktype)
{
    Resolve(TEST_HOST, TEST_PORT, SolidSyslogTransport_Udp);
    LONGS_EQUAL(SOCK_DGRAM, WinsockFake_LastGetAddrInfoSocktype());
}

TEST(SolidSyslogWinsockResolver, TcpTransportPassesStreamSocktype)
{
    Resolve(TEST_HOST, TEST_PORT, SolidSyslogTransport_Tcp);
    LONGS_EQUAL(SOCK_STREAM, WinsockFake_LastGetAddrInfoSocktype());
}

TEST(SolidSyslogWinsockResolver, ReturnsFalseWhenGetAddrInfoFails)
{
    WinsockFake_SetGetAddrInfoFails(true);
    CHECK_FALSE(Resolve(TEST_HOST, TEST_PORT));
}

TEST(SolidSyslogWinsockResolver, DoesNotFreeAddrInfoWhenGetAddrInfoFails)
{
    WinsockFake_SetGetAddrInfoFails(true);
    Resolve(TEST_HOST, TEST_PORT);
    CALLED_FAKE(WinsockFake_FreeAddrInfo, NEVER);
}

TEST(SolidSyslogWinsockResolver, FreesAddrInfoOnSuccess)
{
    Resolve(TEST_HOST, TEST_PORT);
    CALLED_FAKE(WinsockFake_FreeAddrInfo, ONCE);
}
