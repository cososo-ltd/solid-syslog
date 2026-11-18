#include <cstdint>

#include "NorFlashFake.h"
#include "SolidSyslogNorFlash.h"
#include "CppUTest/TestHarness.h"

enum
{
    BLOCK_SIZE = 64,
    BLOCK_COUNT = 4
};

// clang-format off
TEST_GROUP(NorFlashFake)
{
    struct SolidSyslogNorFlash* flash = nullptr;

    void setup() override
    {
        flash = NorFlashFake_Create(BLOCK_SIZE, BLOCK_COUNT);
    }

    [[nodiscard]] bool Program(size_t block, size_t offset, uint8_t value) const
    {
        return SolidSyslogNorFlash_Program(flash, block, offset, &value, 1);
    }

    [[nodiscard]] bool ProgramFour(size_t block, size_t offset) const
    {
        const uint8_t bytes[4] = {0x11, 0x22, 0x33, 0x44};
        return SolidSyslogNorFlash_Program(flash, block, offset, bytes, sizeof(bytes));
    }
};

// clang-format on

TEST(NorFlashFake, ReportsTheGeometryItWasCreatedWith)
{
    LONGS_EQUAL(BLOCK_SIZE, SolidSyslogNorFlash_GetBlockSize(flash));
    LONGS_EQUAL(BLOCK_COUNT, SolidSyslogNorFlash_GetBlockCount(flash));
}

TEST(NorFlashFake, AGeometryOverItsLimitsIsAFlashOfNothing)
{
    flash = NorFlashFake_Create(NORFLASHFAKE_MAX_BLOCK_SIZE + 1, NORFLASHFAKE_MAX_BLOCK_COUNT + 1);

    LONGS_EQUAL(0, SolidSyslogNorFlash_GetBlockSize(flash));
    LONGS_EQUAL(0, SolidSyslogNorFlash_GetBlockCount(flash));
}

TEST(NorFlashFake, ABlankFlashReadsAsAllOnes)
{
    uint8_t byte = 0;

    CHECK_TRUE(SolidSyslogNorFlash_Read(flash, 3, BLOCK_SIZE - 1, &byte, 1));

    BYTES_EQUAL(0xFF, byte);
}

TEST(NorFlashFake, AProgramClearsBits)
{
    CHECK_TRUE(Program(0, 5, 0xF0));

    BYTES_EQUAL(0xF0, NorFlashFake_Peek(0, 5));
}

TEST(NorFlashFake, AProgramSetsNoBit)
{
    CHECK_TRUE(Program(0, 5, 0xF0));

    CHECK_TRUE(Program(0, 5, 0x3F));

    BYTES_EQUAL(0x30, NorFlashFake_Peek(0, 5));
}

TEST(NorFlashFake, AnEraseSetsEveryByteOfItsBlockAndNoOther)
{
    CHECK_TRUE(Program(1, 0, 0x00));
    CHECK_TRUE(Program(1, BLOCK_SIZE - 1, 0x00));
    CHECK_TRUE(Program(2, 0, 0x00));

    CHECK_TRUE(SolidSyslogNorFlash_Erase(flash, 1));

    BYTES_EQUAL(0xFF, NorFlashFake_Peek(1, 0));
    BYTES_EQUAL(0xFF, NorFlashFake_Peek(1, BLOCK_SIZE - 1));
    BYTES_EQUAL(0x00, NorFlashFake_Peek(2, 0));
}

TEST(NorFlashFake, ASpanOutsideABlockIsRefused)
{
    uint8_t bytes[2] = {};

    CHECK_FALSE(SolidSyslogNorFlash_Read(flash, 0, BLOCK_SIZE - 1, bytes, sizeof(bytes)));
    CHECK_FALSE(SolidSyslogNorFlash_Program(flash, 0, BLOCK_SIZE - 1, bytes, sizeof(bytes)));
    CHECK_FALSE(SolidSyslogNorFlash_Read(flash, BLOCK_COUNT, 0, bytes, sizeof(bytes)));
    CHECK_FALSE(SolidSyslogNorFlash_Erase(flash, BLOCK_COUNT));
}

TEST(NorFlashFake, CountsWhatItCarriesOut)
{
    uint8_t byte = 0;
    CHECK_TRUE(Program(0, 0, 0x00));
    CHECK_TRUE(Program(0, 1, 0x00));
    CHECK_TRUE(SolidSyslogNorFlash_Erase(flash, 2));
    CHECK_TRUE(SolidSyslogNorFlash_Read(flash, 0, 0, &byte, 1));

    LONGS_EQUAL(2, NorFlashFake_ProgramCount());
    LONGS_EQUAL(1, NorFlashFake_EraseCount());
    LONGS_EQUAL(1, NorFlashFake_EraseCountOf(2));
    LONGS_EQUAL(0, NorFlashFake_EraseCountOf(1));
    LONGS_EQUAL(1, NorFlashFake_ReadCount());
}

TEST(NorFlashFake, ResetCountsKeepsTheErasesOfEachBlock)
{
    CHECK_TRUE(SolidSyslogNorFlash_Erase(flash, 2));

    NorFlashFake_ResetCounts();

    LONGS_EQUAL(0, NorFlashFake_EraseCount());
    LONGS_EQUAL(1, NorFlashFake_EraseCountOf(2));
}

TEST(NorFlashFake, PokeSetsAByteWhateverItHeld)
{
    CHECK_TRUE(Program(0, 0, 0x00));

    NorFlashFake_Poke(0, 0, 0xA5);

    BYTES_EQUAL(0xA5, NorFlashFake_Peek(0, 0));
}

TEST(NorFlashFake, AFailedOperationFailsOnceAndDoesNothing)
{
    uint8_t byte = 0;
    NorFlashFake_FailNextRead();
    NorFlashFake_FailNextProgram();
    NorFlashFake_FailNextErase();
    NorFlashFake_Poke(1, 0, 0x00);

    CHECK_FALSE(SolidSyslogNorFlash_Read(flash, 0, 0, &byte, 1));
    CHECK_FALSE(Program(0, 0, 0x00));
    CHECK_FALSE(SolidSyslogNorFlash_Erase(flash, 1));

    BYTES_EQUAL(0xFF, NorFlashFake_Peek(0, 0));
    BYTES_EQUAL(0x00, NorFlashFake_Peek(1, 0));
    CHECK_TRUE(SolidSyslogNorFlash_Read(flash, 0, 0, &byte, 1));
    CHECK_TRUE(Program(0, 0, 0x00));
    CHECK_TRUE(SolidSyslogNorFlash_Erase(flash, 1));
}

TEST(NorFlashFake, AReadTouchingAFailedByteFailsAndOthersDoNot)
{
    uint8_t bytes[4] = {};
    NorFlashFake_FailReadsTouching(1, 10);

    CHECK_FALSE(SolidSyslogNorFlash_Read(flash, 1, 8, bytes, sizeof(bytes)));
    CHECK_FALSE(SolidSyslogNorFlash_Read(flash, 1, 10, bytes, 1));
    CHECK_TRUE(SolidSyslogNorFlash_Read(flash, 1, 6, bytes, sizeof(bytes)));
    CHECK_TRUE(SolidSyslogNorFlash_Read(flash, 1, 11, bytes, sizeof(bytes)));
    CHECK_TRUE(SolidSyslogNorFlash_Read(flash, 0, 8, bytes, sizeof(bytes)));
}

TEST(NorFlashFake, AnIgnoredProgramAnswersTrueAndProgramsNothing)
{
    NorFlashFake_IgnoreNextProgram();

    CHECK_TRUE(Program(0, 0, 0x00));

    BYTES_EQUAL(0xFF, NorFlashFake_Peek(0, 0));
    CHECK_TRUE(Program(0, 0, 0x00));
    BYTES_EQUAL(0x00, NorFlashFake_Peek(0, 0));
}

TEST(NorFlashFake, ACleanCutLeavesTheOperationUndone)
{
    NorFlashFake_CutPowerAt(2, NORFLASHFAKE_CUT_CLEAN);
    CHECK_TRUE(ProgramFour(0, 0));

    CHECK_FALSE(ProgramFour(0, 8));

    BYTES_EQUAL(0x11, NorFlashFake_Peek(0, 0));
    BYTES_EQUAL(0xFF, NorFlashFake_Peek(0, 8));
    CHECK_TRUE(NorFlashFake_PowerIsLost());
}

TEST(NorFlashFake, ATornProgramLandsTheFirstHalfOfItsBytes)
{
    NorFlashFake_CutPowerAt(1, NORFLASHFAKE_CUT_TORN);

    CHECK_FALSE(ProgramFour(0, 0));

    BYTES_EQUAL(0x22, NorFlashFake_Peek(0, 1));
    BYTES_EQUAL(0xFF, NorFlashFake_Peek(0, 2));
}

TEST(NorFlashFake, ATornEraseSetsTheFirstHalfOfTheBlock)
{
    CHECK_TRUE(Program(0, 0, 0x00));
    CHECK_TRUE(Program(0, BLOCK_SIZE - 1, 0x00));
    NorFlashFake_CutPowerAt(1, NORFLASHFAKE_CUT_TORN);

    CHECK_FALSE(SolidSyslogNorFlash_Erase(flash, 0));

    BYTES_EQUAL(0xFF, NorFlashFake_Peek(0, 0));
    BYTES_EQUAL(0x00, NorFlashFake_Peek(0, BLOCK_SIZE - 1));
}

TEST(NorFlashFake, ACleanCutLeavesAnEraseUndone)
{
    CHECK_TRUE(Program(0, 0, 0x00));
    NorFlashFake_CutPowerAt(1, NORFLASHFAKE_CUT_CLEAN);

    CHECK_FALSE(SolidSyslogNorFlash_Erase(flash, 0));

    BYTES_EQUAL(0x00, NorFlashFake_Peek(0, 0));
}

TEST(NorFlashFake, NothingAnswersWhilePowerIsLost)
{
    uint8_t byte = 0;
    NorFlashFake_CutPowerAt(1, NORFLASHFAKE_CUT_CLEAN);
    CHECK_FALSE(Program(0, 0, 0x00));

    CHECK_FALSE(SolidSyslogNorFlash_Read(flash, 0, 0, &byte, 1));
    CHECK_FALSE(Program(0, 0, 0x00));
    CHECK_FALSE(SolidSyslogNorFlash_Erase(flash, 0));
}

TEST(NorFlashFake, PowerOnEndsTheOutageAndLeavesThePartAsTheCutLeftIt)
{
    uint8_t byte = 0;
    NorFlashFake_CutPowerAt(1, NORFLASHFAKE_CUT_TORN);
    CHECK_FALSE(ProgramFour(0, 0));

    NorFlashFake_PowerOn();

    CHECK_FALSE(NorFlashFake_PowerIsLost());
    CHECK_TRUE(SolidSyslogNorFlash_Read(flash, 0, 0, &byte, 1));
    BYTES_EQUAL(0x11, byte);
    CHECK_TRUE(Program(0, 8, 0x00));
}

TEST(NorFlashFake, TheCutCountsProgramsAndErasesFromWhenItIsArmed)
{
    CHECK_TRUE(Program(0, 0, 0x00));
    CHECK_TRUE(SolidSyslogNorFlash_Erase(flash, 0));
    LONGS_EQUAL(2, NorFlashFake_WriteOperationCount());
    NorFlashFake_CutPowerAt(2, NORFLASHFAKE_CUT_CLEAN);

    CHECK_TRUE(Program(0, 0, 0x00));
    CHECK_FALSE(SolidSyslogNorFlash_Erase(flash, 0));
}

TEST(NorFlashFake, CreateClearsFaultsCountsAndContent)
{
    CHECK_TRUE(Program(0, 0, 0x00));
    NorFlashFake_FailNextProgram();
    NorFlashFake_CutPowerAt(1, NORFLASHFAKE_CUT_CLEAN);

    flash = NorFlashFake_Create(BLOCK_SIZE, BLOCK_COUNT);

    BYTES_EQUAL(0xFF, NorFlashFake_Peek(0, 0));
    LONGS_EQUAL(0, NorFlashFake_ProgramCount());
    CHECK_TRUE(Program(0, 0, 0x00));
}
