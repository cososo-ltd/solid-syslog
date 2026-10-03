#include "CppUTest/TestHarness.h"

#include <cstdio>

#include "BddTargetVxWorks64.h"
#include "VxWorks64SemFake.h"
#include "VxWorks64TaskFake.h"

// clang-format off
TEST_GROUP(BddTargetVxWorks64)
{
    void setup() override
    {
        VxWorks64TaskFake_Reset();
        VxWorks64SemFake_Reset();
    }

    void teardown() override
    {
        BddTargetVxWorks64_Teardown();
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
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- tmpfile/fclose is C stdio; no owning memory concern
    FILE* input = tmpfile();
    CHECK(input != nullptr);
    (void) fputs("send\nquit\n", input);
    rewind(input);

    BddTargetVxWorks64_RunConsole(input);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- tmpfile/fclose is C stdio; no owning memory concern
    (void) fclose(input);

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
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- tmpfile/fclose is C stdio; no owning memory concern
    FILE* input = tmpfile();
    CHECK(input != nullptr);
    (void) fputs("quit\n", input);
    rewind(input);
    BddTargetVxWorks64_RunConsole(input);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- tmpfile/fclose is C stdio; no owning memory concern
    (void) fclose(input);

    BddTargetVxWorks64_RunService();
}
