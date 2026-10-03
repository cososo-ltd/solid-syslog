#include "CppUTest/TestHarness.h"

#include "VxWorks64SemFake.h"
#include "VxWorks64TaskFake.h"

extern "C"
{
    void BddTargetVxWorks64_Init(void);
}

// clang-format off
TEST_GROUP(BddTargetVxWorks64)
{
    void setup() override
    {
        VxWorks64TaskFake_Reset();
        VxWorks64SemFake_Reset();
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
