#include <stdint.h>

#include "BddTargetServiceThread.h"
#include "BddTargetUdpConfig.h"
#include "SolidSyslog.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogEndpoint.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogPosixMessageQueueBuffer.h"
#include "SolidSyslogGetAddrInfoResolver.h"
#include "SolidSyslogPosixAddress.h"
#include "SolidSyslogPosixDatagram.h"
#include "SolidSyslogUdpSender.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogTunables.h"
#include "SocketFake.h"
#include "ClockFake.h"
#include "SolidSyslogPrival.h"
#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

static int SleepFakeCallCount;
static int lastSleepMs;
static volatile bool* sleepShutdownFlag;

static void SleepFake(int milliseconds)
{
    SleepFakeCallCount++;
    lastSleepMs = milliseconds;
    if (sleepShutdownFlag != nullptr)
    {
        *sleepShutdownFlag = true;
    }
}

static void BddTargetEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    SolidSyslogFormatter_BoundedString(endpoint->Host, BddTargetUdpConfig_GetHost(), SOLIDSYSLOG_MAX_HOST_SIZE);
    endpoint->Port = BddTargetUdpConfig_GetPort();
}

static uint32_t BddTargetEndpointVersion() // NOLINT(modernize-use-trailing-return-type)
{
    return 0;
}

// clang-format off
TEST_GROUP(BddTargetServiceThread)
{
    struct SolidSyslog*         solidSyslog = nullptr;
    struct SolidSyslogSender*   sender   = nullptr;
    struct SolidSyslogBuffer*   buffer   = nullptr;
    struct SolidSyslogStore*    store    = nullptr;
    struct SolidSyslogDatagram* datagram = nullptr;
    struct SolidSyslogResolver* resolver = nullptr;
    struct SolidSyslogAddress*  address  = nullptr;
    volatile bool               shutdown;

    void setup() override
    {
        SocketFake_Reset();
        ClockFake_Reset();
        ClockFake_SetTime(1743768600, 0);
        shutdown          = true;
        SleepFakeCallCount    = 0;
        lastSleepMs       = 0;
        sleepShutdownFlag = nullptr;

        resolver = SolidSyslogGetAddrInfoResolver_Create();
        datagram = SolidSyslogPosixDatagram_Create();
        address  = SolidSyslogPosixAddress_Create();
        SolidSyslogUdpSenderConfig udpConfig = {resolver, datagram, address, BddTargetEndpoint, BddTargetEndpointVersion};
        sender = SolidSyslogUdpSender_Create(&udpConfig);
        buffer = SolidSyslogPosixMessageQueueBuffer_Create(SOLIDSYSLOG_MAX_MESSAGE_SIZE, 10);
        store  = SolidSyslogNullStore_Get();

        SolidSyslogConfig config =
            {buffer, sender, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, store, nullptr, 0};
        solidSyslog = SolidSyslog_Create(&config);
    }

    void teardown() override
    {
        SolidSyslog_Destroy(solidSyslog);
        SolidSyslogPosixMessageQueueBuffer_Destroy(buffer);
        SolidSyslogUdpSender_Destroy(sender);
        SolidSyslogPosixAddress_Destroy(address);
        SolidSyslogPosixDatagram_Destroy(datagram);
        SolidSyslogGetAddrInfoResolver_Destroy(resolver);
    }

    void Log() const
    {
        SolidSyslogMessage message = {SOLIDSYSLOG_FACILITY_LOCAL0, SOLIDSYSLOG_SEVERITY_INFORMATIONAL, nullptr, nullptr};
        SolidSyslog_Log(solidSyslog, &message);
    }
};

// clang-format on

TEST(BddTargetServiceThread, DoesNotSendWhenBufferEmpty)
{
    BddTargetServiceThread_Run(solidSyslog, &shutdown, SleepFake);
    CALLED_FAKE(SocketFake_Sendto, NEVER);
}

TEST(BddTargetServiceThread, YieldsOneMillisecondAfterEachServiceTick)
{
    shutdown = false;
    sleepShutdownFlag = &shutdown;

    BddTargetServiceThread_Run(solidSyslog, &shutdown, SleepFake);

    CALLED_FUNCTION(SleepFake, ONCE);
    LONGS_EQUAL(1, lastSleepMs);
}
