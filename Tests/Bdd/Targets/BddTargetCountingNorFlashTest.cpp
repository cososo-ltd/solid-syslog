#include <cstdint>

#include "BddTargetCountingNorFlash.h"
#include "NorFlashFake.h"
#include "SolidSyslogNorFlash.h"
#include "CppUTest/TestHarness.h"

enum
{
    BLOCK_BYTES = 256,
    BLOCK_COUNT = 8,
    TEXT_BYTES = 80,

    A_BLOCK = 3,
    AN_OFFSET = 7,
    A_BYTE = 0x5A
};

/* Asserts the counts as the flash describes them. A macro, so a failure
 * reports the test's own line. */
#define CHECK_FLASH_COUNTS(expected)                                        \
    {                                                                       \
        CHECK_TRUE(BddTargetCountingNorFlash_Describe(text, sizeof(text))); \
        STRCMP_EQUAL((expected), text);                                     \
    }

// clang-format off
TEST_GROUP(BddTargetCountingNorFlash)
{
    struct SolidSyslogNorFlash* beneath = nullptr;
    struct SolidSyslogNorFlash* counted = nullptr;
    char text[TEXT_BYTES] = {};

    void setup() override
    {
        beneath = NorFlashFake_Create(BLOCK_BYTES, BLOCK_COUNT);
        counted = BddTargetCountingNorFlash_Over(beneath);
    }

    void teardown() override
    {
        BddTargetCountingNorFlash_Over(nullptr);
    }
};

// clang-format on

TEST(BddTargetCountingNorFlash, ThereIsNothingToDescribeWithNoFlashBeneath)
{
    BddTargetCountingNorFlash_Over(nullptr);

    CHECK_FALSE(BddTargetCountingNorFlash_Describe(text, sizeof(text)));
}

TEST(BddTargetCountingNorFlash, AnEraseReachesTheFlashBeneath)
{
    CHECK_TRUE(SolidSyslogNorFlash_Erase(counted, A_BLOCK));

    LONGS_EQUAL(1, NorFlashFake_EraseCountOf(A_BLOCK));
}

TEST(BddTargetCountingNorFlash, AnEraseThatFailsBeneathAnswersFalse)
{
    NorFlashFake_FailNextErase();

    CHECK_FALSE(SolidSyslogNorFlash_Erase(counted, A_BLOCK));
}

TEST(BddTargetCountingNorFlash, AProgramReachesTheFlashBeneath)
{
    const uint8_t byte = A_BYTE;

    CHECK_TRUE(SolidSyslogNorFlash_Program(counted, A_BLOCK, AN_OFFSET, &byte, sizeof(byte)));

    BYTES_EQUAL(A_BYTE, NorFlashFake_Peek(A_BLOCK, AN_OFFSET));
}

TEST(BddTargetCountingNorFlash, AProgramThatFailsBeneathAnswersFalse)
{
    const uint8_t byte = A_BYTE;
    NorFlashFake_FailNextProgram();

    CHECK_FALSE(SolidSyslogNorFlash_Program(counted, A_BLOCK, AN_OFFSET, &byte, sizeof(byte)));
}

TEST(BddTargetCountingNorFlash, AReadAnswersWhatTheFlashBeneathHolds)
{
    uint8_t byte = 0;
    NorFlashFake_Poke(A_BLOCK, AN_OFFSET, A_BYTE);

    CHECK_TRUE(SolidSyslogNorFlash_Read(counted, A_BLOCK, AN_OFFSET, &byte, sizeof(byte)));

    BYTES_EQUAL(A_BYTE, byte);
}

TEST(BddTargetCountingNorFlash, AReadThatFailsBeneathAnswersFalse)
{
    uint8_t byte = 0;
    NorFlashFake_FailNextRead();

    CHECK_FALSE(SolidSyslogNorFlash_Read(counted, A_BLOCK, AN_OFFSET, &byte, sizeof(byte)));
}

TEST(BddTargetCountingNorFlash, ItHasTheBlockSizeOfTheFlashBeneath)
{
    LONGS_EQUAL(BLOCK_BYTES, SolidSyslogNorFlash_GetBlockSize(counted));
}

TEST(BddTargetCountingNorFlash, ItHasTheBlockCountOfTheFlashBeneath)
{
    LONGS_EQUAL(BLOCK_COUNT, SolidSyslogNorFlash_GetBlockCount(counted));
}

TEST(BddTargetCountingNorFlash, ThereIsSomethingToDescribeWithAFlashBeneath)
{
    CHECK_TRUE(BddTargetCountingNorFlash_Describe(text, sizeof(text)));
}

TEST(BddTargetCountingNorFlash, NothingIsCountedUntilSomethingIsAsked)
{
    CHECK_FLASH_COUNTS("flash: erases 0, programs 0, reads 0");
}

TEST(BddTargetCountingNorFlash, AnEraseIsCounted)
{
    SolidSyslogNorFlash_Erase(counted, A_BLOCK);

    CHECK_FLASH_COUNTS("flash: erases 1, programs 0, reads 0");
}

TEST(BddTargetCountingNorFlash, AProgramIsCounted)
{
    const uint8_t byte = A_BYTE;
    SolidSyslogNorFlash_Program(counted, A_BLOCK, AN_OFFSET, &byte, sizeof(byte));

    CHECK_FLASH_COUNTS("flash: erases 0, programs 1, reads 0");
}

TEST(BddTargetCountingNorFlash, AReadIsCounted)
{
    uint8_t byte = 0;
    SolidSyslogNorFlash_Read(counted, A_BLOCK, AN_OFFSET, &byte, sizeof(byte));

    CHECK_FLASH_COUNTS("flash: erases 0, programs 0, reads 1");
}

TEST(BddTargetCountingNorFlash, AFlashPutBeneathStartsTheCountsAgain)
{
    SolidSyslogNorFlash_Erase(counted, A_BLOCK);

    BddTargetCountingNorFlash_Over(beneath);

    CHECK_FLASH_COUNTS("flash: erases 0, programs 0, reads 0");
}
