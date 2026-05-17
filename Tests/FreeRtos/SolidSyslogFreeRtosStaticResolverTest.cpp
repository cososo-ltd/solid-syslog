#include "CppUTest/TestHarness.h"

#include "SolidSyslogAddress.h"
#include "SolidSyslogFreeRtosStaticResolver.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogTransport.h"

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

static const uint8_t TEST_OCTETS[4] = {10, 0, 2, 2};
static const uint16_t TEST_PORT = 514;
static const uint16_t TEST_ALTERNATE_PORT = 9999;
static const char* IGNORED_HOST = "ignored.example.com";

// clang-format off
TEST_GROUP(SolidSyslogFreeRtosStaticResolver)
{
    SolidSyslogFreeRtosStaticResolverStorage storage{};
    struct SolidSyslogResolver*              resolver = nullptr;
    SolidSyslogAddressStorage                addrStorage{};
    struct SolidSyslogAddress*               addr = nullptr;

    void setup() override
    {
        resolver = SolidSyslogFreeRtosStaticResolver_Create(&storage, TEST_OCTETS);
        addr     = SolidSyslogAddress_FromStorage(&addrStorage);
    }

    bool Resolve(const char* host = IGNORED_HOST, uint16_t port = TEST_PORT, enum SolidSyslogTransport transport = SOLIDSYSLOG_TRANSPORT_UDP)
    {
        return SolidSyslogResolver_Resolve(resolver, transport, host, port, addr);
    }

    void RecreateResolverWith(const uint8_t octets[4])
    {
        resolver = SolidSyslogFreeRtosStaticResolver_Create(&storage, octets);
    }

    // NOLINTNEXTLINE(modernize-use-nodiscard) -- used through accessor syntax in tests
    const struct freertos_sockaddr* Result() const
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- platform-layout cast, storage is intptr_t-aligned
        return reinterpret_cast<const struct freertos_sockaddr*>(&addrStorage);
    }
};

// clang-format on

TEST(SolidSyslogFreeRtosStaticResolver, CreateReturnsNonNullResolver)
{
    CHECK(resolver != nullptr);
}

TEST(SolidSyslogFreeRtosStaticResolver, ResolveReturnsTrue)
{
    CHECK_TRUE(Resolve());
}

TEST(SolidSyslogFreeRtosStaticResolver, ResolveSetsSinFamilyToFreeRtosAfInet)
{
    Resolve();
    LONGS_EQUAL(FREERTOS_AF_INET, Result()->sin_family);
}

TEST(SolidSyslogFreeRtosStaticResolver, ResolveWritesIpv4FromCreateOctets)
{
    Resolve();
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(10, 0, 2, 2), Result()->sin_address.ulIP_IPv4);
}

TEST(SolidSyslogFreeRtosStaticResolver, ResolveWritesPortFromArgInNetworkOrder)
{
    Resolve(IGNORED_HOST, TEST_ALTERNATE_PORT);
    LONGS_EQUAL(FreeRTOS_htons(TEST_ALTERNATE_PORT), Result()->sin_port);
}

TEST(SolidSyslogFreeRtosStaticResolver, ResolveProducesSameIpv4ForAnyHostString)
{
    Resolve("first.Host");
    uint32_t firstIpv4 = Result()->sin_address.ulIP_IPv4;

    addrStorage = {};
    Resolve("totally.different.second.host");

    LONGS_EQUAL(firstIpv4, Result()->sin_address.ulIP_IPv4);
}

TEST(SolidSyslogFreeRtosStaticResolver, ResolveProducesSameIpv4ForUdpAndTcpTransport)
{
    Resolve(IGNORED_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_UDP);
    uint32_t udpIpv4 = Result()->sin_address.ulIP_IPv4;

    addrStorage = {};
    Resolve(IGNORED_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_TCP);

    LONGS_EQUAL(udpIpv4, Result()->sin_address.ulIP_IPv4);
}

TEST(SolidSyslogFreeRtosStaticResolver, ResolveWritesAllZeroOctets)
{
    static const uint8_t ZERO_OCTETS[4] = {0, 0, 0, 0};
    RecreateResolverWith(ZERO_OCTETS);
    Resolve();
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(0, 0, 0, 0), Result()->sin_address.ulIP_IPv4);
}

TEST(SolidSyslogFreeRtosStaticResolver, ResolveWritesAllOnesOctets)
{
    static const uint8_t MAX_OCTETS[4] = {255, 255, 255, 255};
    RecreateResolverWith(MAX_OCTETS);
    Resolve();
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(255, 255, 255, 255), Result()->sin_address.ulIP_IPv4);
}

TEST(SolidSyslogFreeRtosStaticResolver, DestroyIsIdempotent)
{
    SolidSyslogFreeRtosStaticResolver_Destroy(resolver);
    SolidSyslogFreeRtosStaticResolver_Destroy(resolver);
}
