#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include <cstdio>
#include <string>

#include "BddTargetVxWorks64.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "TempFile.h"
#include "VxWorks64ClockFake.h"
#include "VxWorks64NetFake.h"
#include "VxWorks64SemFake.h"
#include "VxWorks64TaskFake.h"

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

TEST(BddTargetVxWorks64, InitCreatesAVxWorksMutexForTheBuffer)
{
    BddTargetVxWorks64_Init();

    UNSIGNED_LONGS_EQUAL(1, VxWorks64SemFake_SemMCreateCallCount());
}

TEST(BddTargetVxWorks64, AMessageSentFromTheConsoleTakesTheBufferMutex)
{
    BddTargetVxWorks64_Init();

    RunConsoleWith("send\nquit\n");

    CHECK(VxWorks64SemFake_SemTakeCallCount() > 0U);
    POINTERS_EQUAL(VxWorks64SemFake_LastCreatedId(), VxWorks64SemFake_LastTakenId());
}

TEST(BddTargetVxWorks64, SleepingOneMillisecondDelaysOneTick)
{
    BddTargetVxWorks64_Sleep(1);

    LONGS_EQUAL(1, VxWorks64TaskFake_LastDelayTicks());
}

TEST(BddTargetVxWorks64, SleepingRoundsUpToAWholeTick)
{
    BddTargetVxWorks64_Sleep(20);

    LONGS_EQUAL(2, VxWorks64TaskFake_LastDelayTicks());
}

TEST(BddTargetVxWorks64, SleepingNoTimeOnlyYields)
{
    BddTargetVxWorks64_Sleep(0);

    UNSIGNED_LONGS_EQUAL(1, VxWorks64TaskFake_DelayCount());
    LONGS_EQUAL(0, VxWorks64TaskFake_LastDelayTicks());
}

TEST(BddTargetVxWorks64, TheServiceTaskReturnsOnceTheConsoleHasQuit)
{
    BddTargetVxWorks64_Init();
    RunConsoleWith("quit\n");

    BddTargetVxWorks64_RunService();
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

TEST(BddTargetVxWorks64, ASentMessageSaysItsTimeIsNeitherKnownNorSynchronised)
{
    SendOneMessage();

    STRCMP_CONTAINS("[timeQuality tzKnown=\"0\" isSynced=\"0\"]", VxWorks64NetFake_LastSendtoPayload());
}

TEST(BddTargetVxWorks64, ASentMessageIsStampedWithTheClocksTime)
{
    VxWorks64ClockFake_SetBrokenDownTime(126, 9, 4, 13, 45, 30);

    SendOneMessage();

    STRCMP_CONTAINS(" 2026-10-04T13:45:30.000000Z ", VxWorks64NetFake_LastSendtoPayload());
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
