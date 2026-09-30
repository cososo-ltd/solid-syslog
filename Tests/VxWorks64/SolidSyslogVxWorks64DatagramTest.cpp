#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "vxWorks.h"

#include <netinet/in.h>
#include <sys/socket.h>

#include "SolidSyslogAddress.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogVxWorks64Address.h"
#include "SolidSyslogVxWorks64AddressPrivate.h"
#include "SolidSyslogVxWorks64Datagram.h"
#include "VxWorks64NetFake.h"

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64Datagram)
{
    struct SolidSyslogDatagram* datagram = nullptr;
    struct SolidSyslogAddress* address   = nullptr;
    const char message[6]                = "hello";

    void setup() override
    {
        VxWorks64NetFake_Reset();
        datagram = SolidSyslogVxWorks64Datagram_Create();
        address  = SolidSyslogVxWorks64Address_Create();
        struct sockaddr_in* sin = SolidSyslogVxWorks64Address_AsSockaddrIn(address);
        sin->sin_family         = AF_INET;
        sin->sin_port           = htons(6514U);
        sin->sin_addr.s_addr    = htonl(0x0A00000AU);
    }

    void teardown() override
    {
        SolidSyslogVxWorks64Address_Destroy(address);
        SolidSyslogVxWorks64Datagram_Destroy(datagram);
    }

    enum SolidSyslogDatagramSendResult OpenAndSend()
    {
        SolidSyslogDatagram_Open(datagram);
        return SolidSyslogDatagram_SendTo(datagram, message, sizeof(message) - 1U, address);
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

TEST(SolidSyslogVxWorks64Datagram, SendToSendsTheRecord)
{
    OpenAndSend();

    CALLED_FAKE(VxWorks64NetFake_Sendto, ONCE);
    POINTERS_EQUAL(message, VxWorks64NetFake_LastSendtoBuf());
    LONGS_EQUAL(5, VxWorks64NetFake_LastSendtoLen());
}

TEST(SolidSyslogVxWorks64Datagram, SendToUsesTheSocketOpenMade)
{
    OpenAndSend();

    LONGS_EQUAL(VxWorks64NetFake_SocketFd(), VxWorks64NetFake_LastSendtoFd());
}
