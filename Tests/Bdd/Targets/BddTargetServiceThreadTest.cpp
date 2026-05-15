#include <stdint.h>

#include "BddTargetServiceThread.h"
#include "BddTargetUdpConfig.h"
#include "SolidSyslog.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogEndpoint.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogPosixMessageQueueBuffer.h"
#include "SolidSyslogGetAddrInfoResolver.h"
#include "SolidSyslogPosixDatagram.h"
#include "SolidSyslogUdpSender.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogTunables.h"
#include "SocketFake.h"
#include "ClockFake.h"
#include "SolidSyslogPrival.h"
#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros

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
    SolidSyslogFormatter_BoundedString(endpoint->host, BddTargetUdpConfig_GetHost(), SOLIDSYSLOG_MAX_HOST_SIZE);
    endpoint->port = BddTargetUdpConfig_GetPort();
}

static uint32_t BddTargetEndpointVersion() // NOLINT(modernize-use-trailing-return-type)
{
    return 0;
}

// clang-format off
TEST_GROUP(BddTargetServiceThread)
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
        shutdown          = true;
        SleepFakeCallCount    = 0;
        lastSleepMs       = 0;
        sleepShutdownFlag = nullptr;

        SolidSyslogUdpSenderConfig udpConfig = {SolidSyslogGetAddrInfoResolver_Create(), SolidSyslogPosixDatagram_Create(), BddTargetEndpoint, BddTargetEndpointVersion};
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
        SolidSyslogMessage message = {SolidSyslogFacility_Local0, SolidSyslogSeverity_Informational, nullptr, nullptr};
        SolidSyslog_Log(&message);
    }
};

// clang-format on

TEST(BddTargetServiceThread, DoesNotSendWhenBufferEmpty)
{
    BddTargetServiceThread_Run(&shutdown, SleepFake);
    CALLED_FAKE(SocketFake_Sendto, NEVER);
}

TEST(BddTargetServiceThread, YieldsOneMillisecondAfterEachServiceTick)
{
    shutdown = false;
    sleepShutdownFlag = &shutdown;

    BddTargetServiceThread_Run(&shutdown, SleepFake);

    CALLED_FUNCTION(SleepFake, ONCE);
    LONGS_EQUAL(1, lastSleepMs);
}
