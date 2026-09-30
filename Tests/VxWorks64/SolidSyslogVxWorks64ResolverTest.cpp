#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "vxWorks.h"

#include <netinet/in.h>
#include <sys/socket.h>

#include "SolidSyslogAddress.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogTransport.h"
#include "SolidSyslogVxWorks64Address.h"
#include "SolidSyslogVxWorks64AddressPrivate.h"
#include "SolidSyslogVxWorks64Resolver.h"
#include "VxWorks64NetFake.h"

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64Resolver)
{
    struct SolidSyslogResolver* resolver = nullptr;
    struct SolidSyslogAddress* address   = nullptr;

    void setup() override
    {
        VxWorks64NetFake_Reset();
        resolver = SolidSyslogVxWorks64Resolver_Create();
        address  = SolidSyslogVxWorks64Address_Create();
    }

    bool Resolve(const char* host)
    {
        return SolidSyslogResolver_Resolve(resolver, SOLIDSYSLOG_TRANSPORT_UDP, host, 514U, address);
    }

    const struct sockaddr_in* Resolved() const
    {
        return SolidSyslogVxWorks64Address_AsConstSockaddrIn(address);
    }

    void teardown() override
    {
        SolidSyslogVxWorks64Address_Destroy(address);
        SolidSyslogVxWorks64Resolver_Destroy(resolver);
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64Resolver, ResolvesADottedLiteral)
{
    VxWorks64NetFake_SetInetAddrReturn(0x0100007FUL);

    CHECK_TRUE(SolidSyslogResolver_Resolve(resolver, SOLIDSYSLOG_TRANSPORT_UDP, "127.0.0.1", 514U, address));
}

TEST(SolidSyslogVxWorks64Resolver, PopulatesAddressFamily)
{
    VxWorks64NetFake_SetInetAddrReturn(0x0100007FUL);

    Resolve("127.0.0.1");

    LONGS_EQUAL(AF_INET, Resolved()->sin_family);
}

TEST(SolidSyslogVxWorks64Resolver, PopulatesTheAddressInetAddrParsed)
{
    VxWorks64NetFake_SetInetAddrReturn(0x0100007FUL);

    Resolve("127.0.0.1");

    LONGS_EQUAL(0x0100007FUL, Resolved()->sin_addr.s_addr);
}

TEST(SolidSyslogVxWorks64Resolver, PopulatesThePortInNetworkOrder)
{
    VxWorks64NetFake_SetInetAddrReturn(0x0100007FUL);

    // 6514 rather than 514, whose bytes are equal and so read the same in
    // either order.
    SolidSyslogResolver_Resolve(resolver, SOLIDSYSLOG_TRANSPORT_UDP, "127.0.0.1", 6514U, address);

    LONGS_EQUAL(htons(6514U), Resolved()->sin_port);
}

TEST(SolidSyslogVxWorks64Resolver, ANameIsLookedUpInTheHostLibrary)
{
    VxWorks64NetFake_SetInetAddrReturn((unsigned long) ERROR);
    VxWorks64NetFake_SetHostGetByNameReturn(0x0A00000A);

    Resolve("collector");

    CALLED_FAKE(VxWorks64NetFake_HostGetByName, ONCE);
}
