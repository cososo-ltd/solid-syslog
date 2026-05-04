#include <stdint.h>

#include "ExampleServiceThread.h"
#include "ExampleUdpConfig.h"
#include "SolidSyslog.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogEndpoint.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogPosixMessageQueueBuffer.h"
#include "SolidSyslogGetAddrInfoResolver.h"
#include "SolidSyslogPosixDatagram.h"
#include "SolidSyslogUdpSender.h"
#include "SolidSyslogNullStore.h"
#include "SocketFake.h"
#include "ClockFake.h"
#include "SolidSyslogPrival.h"
#include "CppUTest/TestHarness.h"

static void ExampleEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    SolidSyslogFormatter_BoundedString(endpoint->host, ExampleUdpConfig_GetHost(), SOLIDSYSLOG_MAX_HOST_SIZE);
    endpoint->port = ExampleUdpConfig_GetPort();
}

static uint32_t ExampleEndpointVersion() // NOLINT(modernize-use-trailing-return-type)
{
    return 0;
}

// clang-format off
TEST_GROUP(ExampleServiceThread)
{
    struct SolidSyslogSender* sender = nullptr;
    struct SolidSyslogBuffer* buffer = nullptr;
    struct SolidSyslogStore*  store  = nullptr;
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    volatile bool             shutdown;

    void setup() override
    {
        SocketFake_Reset();
        ClockFake_Reset();
        ClockFake_SetTime(1743768600, 0);
        shutdown = true;

        SolidSyslogUdpSenderConfig udpConfig = {SolidSyslogGetAddrInfoResolver_Create(), SolidSyslogPosixDatagram_Create(), ExampleEndpoint, ExampleEndpointVersion};
        sender = SolidSyslogUdpSender_Create(&udpConfig);
        buffer = SolidSyslogPosixMessageQueueBuffer_Create(SOLIDSYSLOG_MAX_MESSAGE_SIZE, 10);
        store  = SolidSyslogNullStore_Create();

        SolidSyslogConfig config = {buffer, sender, nullptr, nullptr, nullptr, nullptr, store, nullptr, 0};
        SolidSyslog_Create(&config);
    }

    void teardown() override
    {
        SolidSyslog_Destroy();
        SolidSyslogNullStore_Destroy();
        SolidSyslogPosixMessageQueueBuffer_Destroy();
        SolidSyslogUdpSender_Destroy();
        SolidSyslogPosixDatagram_Destroy();
        SolidSyslogGetAddrInfoResolver_Destroy();
    }

    static void Log()
    {
        SolidSyslogMessage message = {SOLIDSYSLOG_FACILITY_LOCAL0, SOLIDSYSLOG_SEVERITY_INFO, nullptr, nullptr};
        SolidSyslog_Log(&message);
    }
};

// clang-format on

TEST(ExampleServiceThread, DoesNotSendWhenBufferEmpty)
{
    ExampleServiceThread_Run(&shutdown);
    LONGS_EQUAL(0, SocketFake_SendtoCallCount());
}
