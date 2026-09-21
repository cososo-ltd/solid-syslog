#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "lwip/sockets.h"

#include "SolidSyslogLwipSocketAddress.h"
#include "SolidSyslogLwipSocketAddressPrivate.h"

// clang-format off
TEST_GROUP(SolidSyslogLwipSocketAddress)
{
    struct SolidSyslogAddress* address = nullptr;

    void setup() override
    {
        address = SolidSyslogLwipSocketAddress_Create();
    }

    void teardown() override
    {
        SolidSyslogLwipSocketAddress_Destroy(address);
    }
};

// clang-format on

TEST(SolidSyslogLwipSocketAddress, CreateReturnsNonNull)
{
    CHECK(address != nullptr);
}

TEST(SolidSyslogLwipSocketAddress, AsSockaddrInRoundTripsBytes)
{
    struct sockaddr_in expected = {};
    expected.sin_family         = AF_INET;
    expected.sin_port           = lwip_htons(514U);
    expected.sin_addr.s_addr    = lwip_htonl(0x7F000001U);

    *SolidSyslogLwipSocketAddress_AsSockaddrIn(address) = expected;

    const struct sockaddr_in* actual = SolidSyslogLwipSocketAddress_AsConstSockaddrIn(address);
    MEMCMP_EQUAL(&expected, actual, sizeof(expected));
}
