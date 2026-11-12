#include "BddTargetLineAssembler.h"
#include "CppUTest/TestHarness.h"

#include <stddef.h>

// clang-format off
TEST_GROUP(BddTargetLineAssembler)
{
    struct BddTargetLineAssembler assembler = {};

    void setup() override
    {
        BddTargetLineAssembler_Reset(&assembler);
    }

    bool Accept(const char* text)
    {
        bool completed = false;
        for (const char* next = text; *next != '\0'; next++)
        {
            completed = BddTargetLineAssembler_Accept(&assembler, *next);
        }
        return completed;
    }
};

// clang-format on

TEST(BddTargetLineAssembler, ACharacterAloneDoesNotCompleteALine)
{
    CHECK_FALSE(BddTargetLineAssembler_Accept(&assembler, 'a'));
}

TEST(BddTargetLineAssembler, ALineFeedCompletesTheLine)
{
    CHECK_TRUE(Accept("send 3\n"));
}

TEST(BddTargetLineAssembler, ACarriageReturnCompletesTheLine)
{
    CHECK_TRUE(Accept("send 3\r"));
}

TEST(BddTargetLineAssembler, TheCompletedLineIsTheTextWithoutItsLineEnd)
{
    Accept("send 3\n");

    STRCMP_EQUAL("send 3", assembler.Text);
}

TEST(BddTargetLineAssembler, TheCompletedLineHasItsLength)
{
    Accept("send 3\n");

    LONGS_EQUAL(6, assembler.Length);
}

TEST(BddTargetLineAssembler, ALineEndWithNothingBeforeItIsNotALine)
{
    CHECK_FALSE(Accept("\n"));
}

TEST(BddTargetLineAssembler, TheSecondLineEndOfAPairIsNotALine)
{
    CHECK_FALSE(Accept("send 3\r\n"));
}

TEST(BddTargetLineAssembler, TheCompletedLineSurvivesTheSecondLineEndOfAPair)
{
    Accept("send 3\r\n");

    STRCMP_EQUAL("send 3", assembler.Text);
}

TEST(BddTargetLineAssembler, TheNextLineStartsAfresh)
{
    Accept("first\n");

    Accept("second\n");

    STRCMP_EQUAL("second", assembler.Text);
}

TEST(BddTargetLineAssembler, ALineLongerThanTheCapacityIsCutToIt)
{
    for (size_t sent = 0; sent < (size_t) BDDTARGET_LINE_ASSEMBLER_CAPACITY + 1U; sent++)
    {
        BddTargetLineAssembler_Accept(&assembler, 'x');
    }

    CHECK_TRUE(BddTargetLineAssembler_Accept(&assembler, '\n'));
    LONGS_EQUAL(BDDTARGET_LINE_ASSEMBLER_CAPACITY, assembler.Length);
}

TEST(BddTargetLineAssembler, ACutLineIsStillTerminated)
{
    for (size_t sent = 0; sent < (size_t) BDDTARGET_LINE_ASSEMBLER_CAPACITY + 1U; sent++)
    {
        BddTargetLineAssembler_Accept(&assembler, 'x');
    }

    BddTargetLineAssembler_Accept(&assembler, '\n');

    BYTES_EQUAL('\0', assembler.Text[BDDTARGET_LINE_ASSEMBLER_CAPACITY]);
}
