#include "CppUTest/TestHarness.h"
#include "SolidSyslogUdpPayload.h"

// clang-format off
TEST_GROUP(SolidSyslogUdpPayload)
{
};

// clang-format on

TEST(SolidSyslogUdpPayload, Ipv4StandardEthernetMtuYields1472)
{
    LONGS_EQUAL(1472, SolidSyslogUdpPayload_FromMtu(1500, false));
}

TEST(SolidSyslogUdpPayload, Ipv6MinimumMtuYields1232)
{
    LONGS_EQUAL(1232, SolidSyslogUdpPayload_FromMtu(1280, true));
}

TEST(SolidSyslogUdpPayload, Ipv6SafePayloadConstantMatchesIpv6MinimumMtu)
{
    LONGS_EQUAL(SolidSyslogUdpPayload_FromMtu(1280, true), SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD);
}

TEST(SolidSyslogUdpPayload, MtuSmallerThanOverheadSaturatesToZero)
{
    LONGS_EQUAL(0, SolidSyslogUdpPayload_FromMtu(0, false));
}
