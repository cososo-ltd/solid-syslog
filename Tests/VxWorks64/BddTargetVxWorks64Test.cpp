#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include <cerrno>
#include <cstdio>
#include <string>

#include <sys/socket.h>

#include "BddTargetStoreSettings.h"
#include "BddTargetVxWorks64.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "TempFile.h"
#include "VxWorks64ClockFake.h"
#include "VxWorks64FsFake.h"
#include "VxWorks64IoFake.h"
#include "VxWorks64NetFake.h"
#include "VxWorks64SemFake.h"
#include "VxWorks64TaskFake.h"

// The buffer's mutex is the first semaphore Init makes, the logger's lock the second.
#define BUFFER_MUTEX() VxWorks64SemFake_CreatedId(0U)
#define LOGGER_LOCK() VxWorks64SemFake_CreatedId(1U)

// clang-format off
TEST_GROUP(BddTargetVxWorks64)
{
    FILE* reports = nullptr;

    void setup() override
    {
        reports = TempFile_Open();
        CHECK(reports != nullptr);
        BddTargetVxWorks64_ReportTo(reports);
        VxWorks64TaskFake_Reset();
        VxWorks64SemFake_Reset();
        VxWorks64NetFake_Reset();
        VxWorks64ClockFake_Reset();
        VxWorks64FsFake_Reset();
        VxWorks64IoFake_Reset();
        // The default collector, 10.0.2.2, as inet_addr answers it.
        VxWorks64NetFake_SetInetAddrReturn(0x0202000AUL);
    }

    void teardown() override
    {
        BddTargetVxWorks64_Teardown();
        BddTargetVxWorks64_ReportTo(nullptr);
        TempFile_Close(reports);
    }

    // Everything the target has reported on its console so far.
    [[nodiscard]] std::string Reported() const
    {
        std::string text;
        (void) fflush(reports);
        (void) fseek(reports, 0L, SEEK_SET);
        char chunk[256];
        size_t count = 0U;
        while ((count = fread(chunk, 1U, sizeof(chunk), reports)) > 0U)
        {
            text.append(chunk, count);
        }
        return text;
    }

    // Everything written to the disk so far.
    [[nodiscard]] static std::string Written()
    {
        return {VxWorks64IoFake_Written(), VxWorks64IoFake_WrittenLength()};
    }

    // The target boots on a file store, logs one message, then quits, and the
    // service task stores it.
    static void StoreOneMessage(const char* settings)
    {
        BddTargetVxWorks64_Init();
        std::string lines = settings;
        lines += "set store file\nsend\nquit\n";
        RunConsoleWith(lines.c_str());
        BddTargetVxWorks64_RunService();
    }

    // Runs the console on these lines, as though typed at the target.
    static void RunConsoleWith(const char* lines)
    {
        FILE* input = TempFile_Open();
        CHECK(input != nullptr);
        if (input != nullptr)
        {
            (void) fputs(lines, input);
            (void) fseek(input, 0L, SEEK_SET);
            BddTargetVxWorks64_RunConsole(input);
            TempFile_Close(input);
        }
    }

    // The target boots, logs one message from its console, then quits, and the
    // service task sends what was logged.
    static void SendOneMessage()
    {
        BddTargetVxWorks64_Init();
        RunConsoleWith("send\nquit\n");
        BddTargetVxWorks64_RunService();
    }
};

// clang-format on

TEST(BddTargetVxWorks64, InitSpawnsTheInteractiveAndServiceTasks)
{
    BddTargetVxWorks64_Init();

    UNSIGNED_LONGS_EQUAL(2, VxWorks64TaskFake_SpawnCount());
}

TEST(BddTargetVxWorks64, InitCreatesAVxWorksMutexForTheBufferAndALockForTheLogger)
{
    BddTargetVxWorks64_Init();

    UNSIGNED_LONGS_EQUAL(2, VxWorks64SemFake_SemMCreateCallCount());
}

TEST(BddTargetVxWorks64, AMessageSentFromTheConsoleTakesTheBufferMutex)
{
    BddTargetVxWorks64_Init();

    RunConsoleWith("send\nquit\n");

    CHECK(VxWorks64SemFake_SemTakeCallCount() > 0U);
    POINTERS_EQUAL(BUFFER_MUTEX(), VxWorks64SemFake_LastTakenId());
}

TEST(BddTargetVxWorks64, TheServiceTaskReturnsOnceTheConsoleHasQuit)
{
    BddTargetVxWorks64_Init();
    RunConsoleWith("quit\n");

    BddTargetVxWorks64_RunService();
}

// `set store file` replaces the logger while the service task runs, so each
// service step holds the lock the rebuild takes.
TEST(BddTargetVxWorks64, EachServiceStepHoldsTheLoggerLock)
{
    BddTargetVxWorks64_Init();
    RunConsoleWith("quit\n");

    BddTargetVxWorks64_RunService();

    POINTERS_EQUAL(LOGGER_LOCK(), VxWorks64SemFake_LastGivenId());
}

TEST(BddTargetVxWorks64, TheServiceTaskSendsWhatTheConsoleLoggedBeforeItStops)
{
    SendOneMessage();

    CALLED_FAKE(VxWorks64NetFake_Sendto, ONCE);
}

TEST(BddTargetVxWorks64, ASentMessageCarriesTheOriginStructuredData)
{
    SendOneMessage();

    STRCMP_CONTAINS("[origin software=\"SolidSyslogBddTarget\"", VxWorks64NetFake_LastSendtoPayload());
}

TEST(BddTargetVxWorks64, ASentMessageCarriesTheFirstSequenceId)
{
    SendOneMessage();

    STRCMP_CONTAINS("[meta sequenceId=\"1\"", VxWorks64NetFake_LastSendtoPayload());
}

TEST(BddTargetVxWorks64, ASentMessageCarriesTheKernelsUpTime)
{
    // 600 ticks at the fake's 60 Hz is ten seconds, in hundredths.
    VxWorks64TaskFake_SetTicks(600U);

    SendOneMessage();

    STRCMP_CONTAINS("sysUpTime=\"1000\"", VxWorks64NetFake_LastSendtoPayload());
}

TEST(BddTargetVxWorks64, ASentMessageCarriesItsLanguage)
{
    SendOneMessage();

    STRCMP_CONTAINS("language=\"en-GB\"", VxWorks64NetFake_LastSendtoPayload());
}

TEST(BddTargetVxWorks64, ASentMessageSaysItsTimeIsKnownAndSynchronised)
{
    SendOneMessage();

    STRCMP_CONTAINS("[timeQuality tzKnown=\"1\" isSynced=\"1\"]", VxWorks64NetFake_LastSendtoPayload());
}

TEST(BddTargetVxWorks64, ASentMessageIsStampedWithTheClocksTime)
{
    VxWorks64ClockFake_SetBrokenDownTime(126, 9, 4, 13, 45, 30);

    SendOneMessage();

    STRCMP_CONTAINS(" 2026-10-04T13:45:30.000000Z ", VxWorks64NetFake_LastSendtoPayload());
}

TEST(BddTargetVxWorks64, ASentMessageCarriesTheHostnameTheTargetSet)
{
    SendOneMessage();

    STRCMP_CONTAINS(" SolidSyslogVxWorks64 ", VxWorks64NetFake_LastSendtoPayload());
}

TEST(BddTargetVxWorks64, SetTimeSetsTheRealTimeClock)
{
    BddTargetVxWorks64_Init();

    RunConsoleWith("set time 1791119130\nquit\n");

    UNSIGNED_LONGS_EQUAL(1791119130UL, VxWorks64ClockFake_LastSetSeconds());
}

TEST(BddTargetVxWorks64, SetTimeRefusesATimeThatIsNotANumber)
{
    BddTargetVxWorks64_Init();

    RunConsoleWith("set time soon\nquit\n");

    UNSIGNED_LONGS_EQUAL(0, VxWorks64ClockFake_SetCallCount());
}

TEST(BddTargetVxWorks64, ALibraryErrorIsReportedOnTheConsole)
{
    static const struct SolidSyslogErrorSource TEST_SOURCE = {"TestSource"};
    BddTargetVxWorks64_Init();

    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_ERROR, &TEST_SOURCE, 3U, 7);

    STRCMP_CONTAINS("[solidsyslog] severity=3 [TestSource cat=3 detail=7]", Reported().c_str());
}

TEST(BddTargetVxWorks64, ATaskThatFailsToStartIsReportedOnTheConsole)
{
    VxWorks64TaskFake_FailSpawns();

    BddTargetVxWorks64_Init();

    STRCMP_CONTAINS("task tSsInteractive failed to start", Reported().c_str());
}

TEST(BddTargetVxWorks64, TheBootsCoreOnlyCheckIsNotReportedAsAnError)
{
    BddTargetVxWorks64_Init();

    CHECK(Reported().find("bad config") == std::string::npos);
}

TEST(BddTargetVxWorks64, TeardownReleasesTheCounterForTheNextBoot)
{
    BddTargetVxWorks64_Init();
    BddTargetVxWorks64_Teardown();

    BddTargetVxWorks64_Init();

    CHECK(Reported().find("[VxWorks64AtomicCounter") == std::string::npos);
}

TEST(BddTargetVxWorks64, SetTransportTcpSendsTheMessageOverTcp)
{
    BddTargetVxWorks64_Init();
    RunConsoleWith("set transport tcp\nsend\nquit\n");
    BddTargetVxWorks64_RunService();
    LONGS_EQUAL(SOCK_STREAM, VxWorks64NetFake_LastSocketType());
    CHECK(VxWorks64NetFake_SendCallCount() > 0U);
    CALLED_FAKE(VxWorks64NetFake_Sendto, NEVER);
}

TEST(BddTargetVxWorks64, SwitchTcpMovesSendingOntoTcp)
{
    BddTargetVxWorks64_Init();
    RunConsoleWith("switch tcp\nsend\nquit\n");
    BddTargetVxWorks64_RunService();
    CHECK(VxWorks64NetFake_SendCallCount() > 0U);
    CALLED_FAKE(VxWorks64NetFake_Sendto, NEVER);
}

TEST(BddTargetVxWorks64, TeardownReleasesTheSendersForTheNextBoot)
{
    BddTargetVxWorks64_Init();
    BddTargetVxWorks64_Teardown();

    BddTargetVxWorks64_Init();

    CHECK(Reported().find("[SwitchingSender") == std::string::npos);
    CHECK(Reported().find("[StreamSender") == std::string::npos);
    CHECK(Reported().find("[VxWorks64TcpStream") == std::string::npos);
}

TEST(BddTargetVxWorks64, AStoreSettingShapesTheFileStore)
{
    BddTargetVxWorks64_Init();

    RunConsoleWith("set max-blocks 2\nquit\n");

    LONGS_EQUAL(2, BddTargetStoreSettings_MaxBlocks());
}

TEST(BddTargetVxWorks64, SetStoreNullLeavesTheDiskAlone)
{
    BddTargetVxWorks64_Init();

    RunConsoleWith("set store null\nquit\n");

    CALLED_FAKE(VxWorks64FsFake_Stat, NEVER);
}

TEST(BddTargetVxWorks64, SetStoreFileReadiesTheDisk)
{
    BddTargetVxWorks64_Init();

    RunConsoleWith("set store file\nquit\n");

    CALLED_FAKE(VxWorks64FsFake_Stat, ONCE);
}

TEST(BddTargetVxWorks64, SetStoreFileKeepsMessagesInFilesOnTheDisk)
{
    StoreOneMessage("");

    STRCMP_CONTAINS("/ata0a/STORE", VxWorks64IoFake_LastOpenName());
    CHECK(VxWorks64IoFake_WrittenLength() > 0U);
}

TEST(BddTargetVxWorks64, SetStoreFileIsRefusedWhenTheDiskCannotBeReadied)
{
    VxWorks64FsFake_FailFormats();

    StoreOneMessage("");

    CALLED_FAKE(VxWorks64IoFake_Open, NEVER);
}

TEST(BddTargetVxWorks64, SetStoreFileIsRefusedForAPolicyThisTargetDoesNotCarry)
{
    StoreOneMessage("set security-policy hmac-sha256\n");

    CALLED_FAKE(VxWorks64FsFake_Stat, NEVER);
    CALLED_FAKE(VxWorks64IoFake_Open, NEVER);
}

TEST(BddTargetVxWorks64, AStoredMessageCarriesTheOriginStructuredData)
{
    StoreOneMessage("");

    CHECK(Written().find("[origin") != std::string::npos);
}

TEST(BddTargetVxWorks64, NoSdStoresTheMetaStructuredDataAlone)
{
    StoreOneMessage("set no-sd 1\n");

    CHECK(Written().find("[meta") != std::string::npos);
    CHECK(Written().find("[origin") == std::string::npos);
}

TEST(BddTargetVxWorks64, TeardownReleasesTheFileStoreForTheNextBoot)
{
    StoreOneMessage("");
    BddTargetVxWorks64_Teardown();

    StoreOneMessage("");

    CHECK(Reported().find("cat=") == std::string::npos);
}

TEST(BddTargetVxWorks64, ABootForgetsTheStoreSettingsOfTheLastBoot)
{
    BddTargetVxWorks64_Init();
    RunConsoleWith("set max-blocks 2\nquit\n");
    BddTargetVxWorks64_Teardown();

    BddTargetVxWorks64_Init();

    LONGS_EQUAL(10, BddTargetStoreSettings_MaxBlocks());
}

TEST(BddTargetVxWorks64, CrossingTheCapacityThresholdIsReportedOnTheConsole)
{
    StoreOneMessage("set capacity-threshold 1\n");

    STRCMP_CONTAINS("[THRESHOLD-CROSSED]", Reported().c_str());
}

TEST(BddTargetVxWorks64, AFullStoreUnderHaltWithHaltExitEndsTheRunWithStatusTwo)
{
    VxWorks64NetFake_FailConnectWithErrno(ECONNREFUSED);
    BddTargetVxWorks64_Init();

    RunConsoleWith("set max-blocks 2\nset max-block-size 520\nset discard-policy halt\nset halt-exit 1\n"
                   "set transport tcp\nset store file\nsend 10\nquit\n");
    BddTargetVxWorks64_RunService();

    STRCMP_CONTAINS("[EXIT 2]", Reported().c_str());
}

TEST(BddTargetVxWorks64, AFullStoreUnderHaltWithoutHaltExitKeepsTheRunGoing)
{
    VxWorks64NetFake_FailConnectWithErrno(ECONNREFUSED);
    BddTargetVxWorks64_Init();

    RunConsoleWith("set max-blocks 2\nset max-block-size 520\nset discard-policy halt\n"
                   "set transport tcp\nset store file\nsend 10\nquit\n");
    BddTargetVxWorks64_RunService();

    CHECK(Reported().find("[EXIT") == std::string::npos);
}
