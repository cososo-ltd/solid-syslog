#include <stddef.h>

#include "CppUTest/TestHarness.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogUdpPayload.h"

// clang-format off
TEST_GROUP(SolidSyslogNullDatagram)
{
    struct SolidSyslogDatagram* datagram = nullptr;

    void setup() override
    {
        datagram = SolidSyslogNullDatagram_Get();
    }
};

// clang-format on

TEST(SolidSyslogNullDatagram, SendToReturnsSentToDropOnTheFloor)
{
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT, SolidSyslogDatagram_SendTo(datagram, "x", 1, nullptr));
}

TEST(SolidSyslogNullDatagram, OpenReturnsTrue)
{
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogNullDatagram, MaxPayloadReturnsIpv6SafeDefault)
{
    UNSIGNED_LONGS_EQUAL((size_t) SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogNullDatagram, CloseDoesNotCrash)
{
    SolidSyslogDatagram_Close(datagram);
}
