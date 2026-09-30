#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "vxWorks.h"

#include <sys/socket.h>

#include "SolidSyslogDatagram.h"
#include "SolidSyslogVxWorks64Datagram.h"
#include "VxWorks64NetFake.h"

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64Datagram)
{
    struct SolidSyslogDatagram* datagram = nullptr;

    void setup() override
    {
        VxWorks64NetFake_Reset();
        datagram = SolidSyslogVxWorks64Datagram_Create();
    }

    void teardown() override
    {
        SolidSyslogVxWorks64Datagram_Destroy(datagram);
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64Datagram, OpenMakesOneSocket)
{
    SolidSyslogDatagram_Open(datagram);

    CALLED_FAKE(VxWorks64NetFake_Socket, ONCE);
}

TEST(SolidSyslogVxWorks64Datagram, OpenAsksForAnIpv4DatagramSocket)
{
    SolidSyslogDatagram_Open(datagram);

    LONGS_EQUAL(AF_INET, VxWorks64NetFake_LastSocketDomain());
    LONGS_EQUAL(SOCK_DGRAM, VxWorks64NetFake_LastSocketType());
    LONGS_EQUAL(0, VxWorks64NetFake_LastSocketProtocol());
}

TEST(SolidSyslogVxWorks64Datagram, OpenFailsWhenTheStackCannotMakeASocket)
{
    VxWorks64NetFake_SetSocketFails(true);

    CHECK_FALSE(SolidSyslogDatagram_Open(datagram));
}
