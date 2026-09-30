#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "vxWorks.h"

#include <netinet/in.h>
#include <sys/socket.h>

#include "SolidSyslogVxWorks64Address.h"
#include "SolidSyslogVxWorks64AddressPrivate.h"

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64Address)
{
    struct SolidSyslogAddress* address = nullptr;

    void setup() override
    {
        address = SolidSyslogVxWorks64Address_Create();
    }

    void teardown() override
    {
        SolidSyslogVxWorks64Address_Destroy(address);
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64Address, CreateReturnsNonNull)
{
    CHECK(address != nullptr);
}

TEST(SolidSyslogVxWorks64Address, AsSockaddrInRoundTripsBytes)
{
    struct sockaddr_in expected = {};
    expected.sin_family = AF_INET;
    expected.sin_port = htons(514U);
    expected.sin_addr.s_addr = htonl(0x7F000001U);

    *SolidSyslogVxWorks64Address_AsSockaddrIn(address) = expected;

    const struct sockaddr_in* actual = SolidSyslogVxWorks64Address_AsConstSockaddrIn(address);
    MEMCMP_EQUAL(&expected, actual, sizeof(expected));
}
