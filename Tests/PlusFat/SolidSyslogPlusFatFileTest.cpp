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
    char buffer[5] = {};

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

TEST(SolidSyslogPlusFatFile, ReadReturnsRequestedBytes)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    const char source[5] = {'h', 'e', 'l', 'l', 'o'};
    PlusFatFake_SetReadSource(source, sizeof(source));

    CHECK_TRUE(SolidSyslogFile_Read(file, buffer, sizeof(buffer)));

    MEMCMP_EQUAL(source, buffer, sizeof(source));
}

TEST(SolidSyslogPlusFatFile, ReadCallsFfreadWithUnitSizeAndCount)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    const char source[5] = {'h', 'e', 'l', 'l', 'o'};
    PlusFatFake_SetReadSource(source, sizeof(source));

    SolidSyslogFile_Read(file, buffer, sizeof(buffer));

    CALLED_FAKE(PlusFatFake_Read, ONCE);
    UNSIGNED_LONGS_EQUAL(1, PlusFatFake_LastReadSize());
    UNSIGNED_LONGS_EQUAL(sizeof(buffer), PlusFatFake_LastReadItems());
}

TEST(SolidSyslogPlusFatFile, ReadFailsWhenFewerBytesAvailable)
{
    SolidSyslogFile_Open(file, TEST_PATH);
    const char source[3] = {'a', 'b', 'c'};
    PlusFatFake_SetReadSource(source, sizeof(source));

    CHECK_FALSE(SolidSyslogFile_Read(file, buffer, sizeof(buffer)));
}
