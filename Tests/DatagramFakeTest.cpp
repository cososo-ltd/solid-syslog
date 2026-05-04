#include "DatagramFake.h"
#include "SolidSyslogDatagram.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(DatagramFake)
{
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogDatagram* datagram = nullptr;

    // cppcheck-suppress unreadVariable -- read by teardown and tests; cppcheck does not model CppUTest lifecycle
    void setup() override { datagram = DatagramFake_Create(); }
    void teardown() override { DatagramFake_Destroy(datagram); }
};

// clang-format on

TEST(DatagramFake, CreateSucceeds)
{
    CHECK_TRUE(datagram != nullptr);
}

TEST(DatagramFake, OpenIncrementsCount)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(1, DatagramFake_OpenCallCount(datagram));
}

TEST(DatagramFake, OpenReturnsTrue)
{
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));
}

TEST(DatagramFake, SendIncrementsCount)
{
    const char payload[] = "hi";
    SolidSyslogDatagram_SendTo(datagram, payload, sizeof(payload), nullptr);
    LONGS_EQUAL(1, DatagramFake_SendCallCount(datagram));
}

TEST(DatagramFake, SendDefaultsToSent)
{
    const char payload[] = "hi";
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SENT, SolidSyslogDatagram_SendTo(datagram, payload, sizeof(payload), nullptr));
}

TEST(DatagramFake, SendReturnsConfiguredResultPerCall)
{
    DatagramFake_SetSendResult(datagram, 0, SOLIDSYSLOG_DATAGRAM_OVERSIZE);
    DatagramFake_SetSendResult(datagram, 1, SOLIDSYSLOG_DATAGRAM_FAILED);
    const char payload[] = "hi";
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_OVERSIZE, SolidSyslogDatagram_SendTo(datagram, payload, sizeof(payload), nullptr));
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_FAILED, SolidSyslogDatagram_SendTo(datagram, payload, sizeof(payload), nullptr));
}

TEST(DatagramFake, SendCapturesBufferAndSize)
{
    const char payload[] = "hello";
    SolidSyslogDatagram_SendTo(datagram, payload, sizeof(payload), nullptr);
    POINTERS_EQUAL(payload, DatagramFake_SendBuffer(datagram, 0));
    LONGS_EQUAL(sizeof(payload), DatagramFake_SendSize(datagram, 0));
}

TEST(DatagramFake, MaxPayloadIncrementsCount)
{
    SolidSyslogDatagram_MaxPayload(datagram);
    LONGS_EQUAL(1, DatagramFake_MaxPayloadCallCount(datagram));
}

TEST(DatagramFake, MaxPayloadReturnsConfiguredValue)
{
    DatagramFake_SetMaxPayload(datagram, 1232);
    LONGS_EQUAL(1232, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(DatagramFake, CloseIncrementsCount)
{
    SolidSyslogDatagram_Close(datagram);
    LONGS_EQUAL(1, DatagramFake_CloseCallCount(datagram));
}
