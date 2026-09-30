#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "vxWorks.h"

#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "SolidSyslogAddress.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogUdpPayload.h"
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

TEST(SolidSyslogVxWorks64Datagram, SendToAddressesTheResolvedDestination)
{
    OpenAndSend();

    POINTERS_EQUAL(SolidSyslogVxWorks64Address_AsConstSockaddrIn(address), VxWorks64NetFake_LastSendtoTo());
    LONGS_EQUAL(sizeof(struct sockaddr_in), VxWorks64NetFake_LastSendtoToLen());
}

TEST(SolidSyslogVxWorks64Datagram, SendToReportsSentWhenTheStackTakesTheRecord)
{
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT, OpenAndSend());
}

TEST(SolidSyslogVxWorks64Datagram, SendToReportsOversizeWhenTheStackRefusesTheSize)
{
    VxWorks64NetFake_FailSendtoWithErrno(EMSGSIZE);

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE, OpenAndSend());
}

TEST(SolidSyslogVxWorks64Datagram, SendToReportsFailedForAnyOtherRefusal)
{
    VxWorks64NetFake_FailSendtoWithErrno(ENETUNREACH);

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED, OpenAndSend());
}

TEST(SolidSyslogVxWorks64Datagram, SendToPassesNoFlags)
{
    OpenAndSend();

    LONGS_EQUAL(0, VxWorks64NetFake_LastSendtoFlags());
}

TEST(SolidSyslogVxWorks64Datagram, MaxPayloadIsTheUnknownPathPayload)
{
    SolidSyslogDatagram_Open(datagram);

    LONGS_EQUAL(SolidSyslogUdpPayload_UnknownPath(false), SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogVxWorks64Datagram, CloseClosesTheSocketOpenMade)
{
    SolidSyslogDatagram_Open(datagram);

    SolidSyslogDatagram_Close(datagram);

    CALLED_FAKE(VxWorks64NetFake_Close, ONCE);
    LONGS_EQUAL(VxWorks64NetFake_SocketFd(), VxWorks64NetFake_LastClosedFd());
}

// The Datagram contract: Close is idempotent and safe on an unopened datagram,
// because the caller's failure paths call it.
TEST(SolidSyslogVxWorks64Datagram, CloseOnAnUnopenedDatagramClosesNothing)
{
    SolidSyslogDatagram_Close(datagram);

    CALLED_FAKE(VxWorks64NetFake_Close, NEVER);
}

TEST(SolidSyslogVxWorks64Datagram, CloseTwiceClosesOnce)
{
    SolidSyslogDatagram_Open(datagram);

    SolidSyslogDatagram_Close(datagram);
    SolidSyslogDatagram_Close(datagram);

    CALLED_FAKE(VxWorks64NetFake_Close, ONCE);
}

TEST(SolidSyslogVxWorks64Datagram, DestroyClosesAnOpenSocket)
{
    SolidSyslogDatagram_Open(datagram);

    SolidSyslogVxWorks64Datagram_Destroy(datagram);
    datagram = nullptr;

    CALLED_FAKE(VxWorks64NetFake_Close, ONCE);
}
