#include "CppUTest/TestHarness.h"
#include "TestUtils.h"

extern "C"
{
#include "PlusFatFake.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogPlusFatFile.h"
#include "ff_stdio.h"
}

using namespace CososoTesting;

static const char* const TEST_PATH = "test.log";

// clang-format off
TEST_GROUP(SolidSyslogPlusFatFile)
{
    struct SolidSyslogFile* file = nullptr;

    void setup() override
    {
        PlusFatFake_Reset();
        file = SolidSyslogPlusFatFile_Create();
    }

    void teardown() override
    {
        SolidSyslogPlusFatFile_Destroy(file);
    }
};

// clang-format on

TEST(SolidSyslogPlusFatFile, CreateSucceeds)
{
    CHECK(file != nullptr);
}

TEST(SolidSyslogPlusFatFile, OpenSucceeds)
{
    CHECK_TRUE(SolidSyslogFile_Open(file, TEST_PATH));
    CHECK_TRUE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogPlusFatFile, OpenFallsBackToCreateModeWhenReadPlusFails)
{
    PlusFatFake_SetOpenFailsForMode("r+");

    CHECK_TRUE(SolidSyslogFile_Open(file, TEST_PATH));

    LONGS_EQUAL(2, PlusFatFake_OpenCallCount());
    STRCMP_EQUAL(TEST_PATH, PlusFatFake_LastOpenPath());
    STRCMP_EQUAL("r+", PlusFatFake_OpenModeAt(0));
    STRCMP_EQUAL("w+", PlusFatFake_OpenModeAt(1));
}

TEST(SolidSyslogPlusFatFile, OpenFailsWhenBothModesFail)
{
    PlusFatFake_SetOpenAlwaysFails();

    CHECK_FALSE(SolidSyslogFile_Open(file, TEST_PATH));
    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogPlusFatFile, CloseCallsFfcloseAndClearsIsOpen)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogFile_Close(file);

    CALLED_FAKE(PlusFatFake_Close, ONCE);
    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogPlusFatFile, CloseIsNoOpWhenAlreadyClosed)
{
    SolidSyslogFile_Close(file);

    CALLED_FAKE(PlusFatFake_Close, NEVER);
}

TEST(SolidSyslogPlusFatFile, DestroyClosesOpenFile)
{
    SolidSyslogFile_Open(file, TEST_PATH);

    SolidSyslogPlusFatFile_Destroy(file);
    file = nullptr;

    CALLED_FAKE(PlusFatFake_Close, ONCE);
}
