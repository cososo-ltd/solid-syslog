#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include <cstdio>

#include "BddTargetVxWorks64.h"
#include "TempFile.h"
#include "VxWorks64NetFake.h"
#include "VxWorks64SemFake.h"
#include "VxWorks64TaskFake.h"

// clang-format off
TEST_GROUP(BddTargetVxWorks64)
{
    void setup() override
    {
        VxWorks64TaskFake_Reset();
        VxWorks64SemFake_Reset();
        VxWorks64NetFake_Reset();
        // The default collector, 10.0.2.2, as inet_addr answers it.
        VxWorks64NetFake_SetInetAddrReturn(0x0202000AUL);
    }

    void teardown() override
    {
        BddTargetVxWorks64_Teardown();
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
    BddTargetVxWorks64_Init();
    RunConsoleWith("send\nquit\n");

    BddTargetVxWorks64_RunService();

    CALLED_FAKE(VxWorks64NetFake_Sendto, ONCE);
}

TEST(BddTargetVxWorks64, ASentMessageCarriesTheOriginStructuredData)
{
    BddTargetVxWorks64_Init();
    RunConsoleWith("send\nquit\n");

    BddTargetVxWorks64_RunService();

    STRCMP_CONTAINS("[origin software=\"SolidSyslogBddTarget\"", VxWorks64NetFake_LastSendtoPayload());
}
