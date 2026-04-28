#include "CppUTest/TestHarness.h"
#include "SolidSyslogUdpPayload.h"
#include <cstdint>

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

TEST(SolidSyslogUdpPayload, TrimToCodepointBoundaryAsciiCutNeedsNoWalk)
{
    const uint8_t buffer[] = "hello";
    LONGS_EQUAL(5, SolidSyslogUdpPayload_TrimToCodepointBoundary(buffer, 5));
}

/* 'é' = 0xC3 0xA9 (2-byte codepoint). Cut after the start byte walks back 1. */
TEST(SolidSyslogUdpPayload, TrimToCodepointBoundaryMidTwoByteWalksBackOne)
{
    const uint8_t buffer[] = {0xC3, 0xA9};
    LONGS_EQUAL(0, SolidSyslogUdpPayload_TrimToCodepointBoundary(buffer, 1));
}

/* '€' = 0xE2 0x82 0xAC (3-byte codepoint). Cut after start byte (length=1)
 * walks back 1; cut after first continuation (length=2) walks back 2. */
TEST(SolidSyslogUdpPayload, TrimToCodepointBoundaryMidThreeByteAfterStartWalksBackOne)
{
    const uint8_t buffer[] = {0xE2, 0x82, 0xAC};
    LONGS_EQUAL(0, SolidSyslogUdpPayload_TrimToCodepointBoundary(buffer, 1));
}

TEST(SolidSyslogUdpPayload, TrimToCodepointBoundaryMidThreeByteAfterFirstContinuationWalksBackTwo)
{
    const uint8_t buffer[] = {0xE2, 0x82, 0xAC};
    LONGS_EQUAL(0, SolidSyslogUdpPayload_TrimToCodepointBoundary(buffer, 2));
}

/* '😀' = 0xF0 0x9F 0x98 0x80 (4-byte codepoint). Cuts after byte 1/2/3 walk back 1/2/3. */
TEST(SolidSyslogUdpPayload, TrimToCodepointBoundaryMidFourByteAfterStartWalksBackOne)
{
    const uint8_t buffer[] = {0xF0, 0x9F, 0x98, 0x80};
    LONGS_EQUAL(0, SolidSyslogUdpPayload_TrimToCodepointBoundary(buffer, 1));
}

TEST(SolidSyslogUdpPayload, TrimToCodepointBoundaryMidFourByteAfterFirstContinuationWalksBackTwo)
{
    const uint8_t buffer[] = {0xF0, 0x9F, 0x98, 0x80};
    LONGS_EQUAL(0, SolidSyslogUdpPayload_TrimToCodepointBoundary(buffer, 2));
}

TEST(SolidSyslogUdpPayload, TrimToCodepointBoundaryMidFourByteAfterSecondContinuationWalksBackThree)
{
    const uint8_t buffer[] = {0xF0, 0x9F, 0x98, 0x80};
    LONGS_EQUAL(0, SolidSyslogUdpPayload_TrimToCodepointBoundary(buffer, 3));
}

/* Cut exactly on a codepoint boundary — no walk back. */
TEST(SolidSyslogUdpPayload, TrimToCodepointBoundaryExactTwoByteBoundary)
{
    const uint8_t buffer[] = {0xC3, 0xA9};
    LONGS_EQUAL(2, SolidSyslogUdpPayload_TrimToCodepointBoundary(buffer, 2));
}

TEST(SolidSyslogUdpPayload, TrimToCodepointBoundaryExactFourByteBoundary)
{
    const uint8_t buffer[] = {0xF0, 0x9F, 0x98, 0x80};
    LONGS_EQUAL(4, SolidSyslogUdpPayload_TrimToCodepointBoundary(buffer, 4));
}

TEST(SolidSyslogUdpPayload, TrimToCodepointBoundaryZeroLengthReturnsZero)
{
    const uint8_t buffer[] = {0xC3, 0xA9};
    LONGS_EQUAL(0, SolidSyslogUdpPayload_TrimToCodepointBoundary(buffer, 0));
}

/* Buffer is one giant codepoint and the cut would land on its boundary. */
TEST(SolidSyslogUdpPayload, TrimToCodepointBoundarySingleCodepointAtBoundaryKeepsLength)
{
    const uint8_t buffer[] = {0xE2, 0x82, 0xAC};
    LONGS_EQUAL(3, SolidSyslogUdpPayload_TrimToCodepointBoundary(buffer, 3));
}
