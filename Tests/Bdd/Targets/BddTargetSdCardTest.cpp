#include "BddTargetSdCard.h"
#include "CppUTest/TestHarness.h"
#include "FakeSdCard.h"

#include <cstddef>
#include <cstdint>
#include <vector>

enum
{
    SEND_STATUS = 13,
    SET_BLOCKLEN = 16,
    READ_SINGLE_BLOCK = 17,
    WRITE_BLOCK = 24,

    A_SECTOR = 3,
    ILLEGAL_COMMAND = 0x04,
    OUT_OF_RANGE_TOKEN = 0x08,
    DATA_REJECTED_WRITE_ERROR = 0xED,
    WRITE_PROTECT_VIOLATION = 0x02
};

#define CHECK_RESULT(expected, actual) LONGS_EQUAL((expected), (actual))

#define CHECK_SECTORS_EQUAL(expected, actual) MEMCMP_EQUAL((expected).data(), (actual).data(), FakeSdCard::SECTOR_SIZE)

static FakeSdCard::Sector SectorOfPattern(uint8_t first)
{
    FakeSdCard::Sector sector = {};
    for (size_t i = 0; i < sector.size(); i++)
    {
        sector[i] = static_cast<uint8_t>(first + i);
    }
    return sector;
}

// clang-format off
TEST_GROUP(BddTargetSdCard)
{
    FakeSdCard::Sector buffer = {};

    void setup() override
    {
        FakeSdCard::Reset();
        BddTargetSdCard_Stop();
    }

    void teardown() override
    {
        /* The fake holds what it was sent in memory the leak check counts. */
        FakeSdCard::Reset();
    }

    static void Started()
    {
        CHECK_RESULT(BDD_TARGET_SD_CARD_OK, BddTargetSdCard_Start());
    }

    BddTargetSdCardResult Read(uint32_t sector)
    {
        return BddTargetSdCard_Read(sector, buffer.data());
    }

    static BddTargetSdCardResult Write(uint32_t sector, const FakeSdCard::Sector& content)
    {
        return BddTargetSdCard_Write(sector, content.data());
    }
};

// clang-format on

TEST(BddTargetSdCard, IsNotStartedUntilItIsStarted)
{
    CHECK_FALSE(BddTargetSdCard_IsStarted());
    LONGS_EQUAL(0, BddTargetSdCard_SectorCount());
}

TEST(BddTargetSdCard, ReadsNothingBeforeItIsStarted)
{
    CHECK_RESULT(BDD_TARGET_SD_CARD_NOT_STARTED, Read(A_SECTOR));
    LONGS_EQUAL(0, FakeSdCard::Exchanges());
}

TEST(BddTargetSdCard, WritesNothingBeforeItIsStarted)
{
    CHECK_RESULT(BDD_TARGET_SD_CARD_NOT_STARTED, Write(A_SECTOR, SectorOfPattern(1)));
    LONGS_EQUAL(0, FakeSdCard::Exchanges());
}

TEST(BddTargetSdCard, StartsAHighCapacityCard)
{
    CHECK_RESULT(BDD_TARGET_SD_CARD_OK, BddTargetSdCard_Start());
    CHECK_TRUE(BddTargetSdCard_IsStarted());
}

TEST(BddTargetSdCard, SaysNothingAnsweredWhenThereIsNoCard)
{
    FakeSdCard::settings.present = false;
    CHECK_RESULT(BDD_TARGET_SD_CARD_NO_ANSWER, BddTargetSdCard_Start());
    CHECK_FALSE(BddTargetSdCard_IsStarted());
}

TEST(BddTargetSdCard, ClocksTheCardAwakeWithItsSelectHighBeforeTheFirstCommand)
{
    Started();
    CHECK_TRUE(FakeSdCard::ClocksBeforeFirstCommand() >= 74);
}

TEST(BddTargetSdCard, StartsTheCardAtTheStartRate)
{
    Started();
    STRCMP_EQUAL("start", FakeSdCard::RateAtFirstCommand().c_str());
}

TEST(BddTargetSdCard, RunsAStartedCardAtTheRunRate)
{
    Started();
    STRCMP_EQUAL("run", FakeSdCard::Rates().back().c_str());
}

TEST(BddTargetSdCard, LeavesTheBusAtTheStartRateWhenTheCardDoesNotStart)
{
    FakeSdCard::settings.present = false;
    BddTargetSdCard_Start();
    STRCMP_EQUAL("start", FakeSdCard::Rates().back().c_str());
}

TEST(BddTargetSdCard, SendsGoIdleFirstWithTheCrcTheSpecificationGivesForIt)
{
    Started();
    const std::vector<uint8_t> expected = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
    CHECK_TRUE(expected == FakeSdCard::FirstFrame());
}

TEST(BddTargetSdCard, AsksAgainWhenTheCardLetsItsFirstCommandsPass)
{
    FakeSdCard::settings.goIdlesIgnored = 3;
    CHECK_RESULT(BDD_TARGET_SD_CARD_OK, BddTargetSdCard_Start());
}

TEST(BddTargetSdCard, GivesUpOnACardThatLetsEveryGoIdlePass)
{
    FakeSdCard::settings.goIdlesIgnored = 1000;
    CHECK_RESULT(BDD_TARGET_SD_CARD_NO_ANSWER, BddTargetSdCard_Start());
}

TEST(BddTargetSdCard, TurnsTheCardsCrcCheckingOn)
{
    Started();
    CHECK_TRUE(FakeSdCard::CrcIsOn());
}

TEST(BddTargetSdCard, RefusesACardOfTheFirstVersion)
{
    FakeSdCard::settings.versionOne = true;
    CHECK_RESULT(BDD_TARGET_SD_CARD_UNSUPPORTED, BddTargetSdCard_Start());
    CHECK_FALSE(BddTargetSdCard_IsStarted());
}

TEST(BddTargetSdCard, RefusesACardThatSendsBackAnotherCheckPattern)
{
    FakeSdCard::settings.checkPattern = 0x55;
    CHECK_RESULT(BDD_TARGET_SD_CARD_UNSUPPORTED, BddTargetSdCard_Start());
}

TEST(BddTargetSdCard, WaitsForACardThatIsSlowToStart)
{
    FakeSdCard::settings.startPolls = 50;
    CHECK_RESULT(BDD_TARGET_SD_CARD_OK, BddTargetSdCard_Start());
}

TEST(BddTargetSdCard, TimesOutOnACardThatNeverStarts)
{
    FakeSdCard::settings.neverStarts = true;
    CHECK_RESULT(BDD_TARGET_SD_CARD_TIMED_OUT, BddTargetSdCard_Start());
    CHECK_FALSE(BddTargetSdCard_IsStarted());
}

TEST(BddTargetSdCard, TimesOutOnACardThatHoldsItsDataLineLow)
{
    FakeSdCard::settings.stuckBusy = true;
    CHECK_RESULT(BDD_TARGET_SD_CARD_TIMED_OUT, BddTargetSdCard_Start());
}

TEST(BddTargetSdCard, LeavesTheCardDeselected)
{
    Started();
    CHECK_FALSE(FakeSdCard::IsSelected());
}

TEST(BddTargetSdCard, ReportsTheCapacityOfAHighCapacityCard)
{
    FakeSdCard::SetHighCapacitySectors(15523840U);
    Started();
    LONGS_EQUAL(15523840U, BddTargetSdCard_SectorCount());
}

TEST(BddTargetSdCard, ReportsTheCapacityOfAStandardCapacityCard)
{
    FakeSdCard::settings.highCapacity = false;
    FakeSdCard::SetStandardCapacityTwoGigabytes();
    Started();
    LONGS_EQUAL(4194304U, BddTargetSdCard_SectorCount());
}

TEST(BddTargetSdCard, DoesNotStartACardWhoseCapacityArrivesDamaged)
{
    FakeSdCard::settings.corruptsDataItSends = true;
    CHECK_RESULT(BDD_TARGET_SD_CARD_BAD_DATA, BddTargetSdCard_Start());
    CHECK_FALSE(BddTargetSdCard_IsStarted());
}

TEST(BddTargetSdCard, CanBeStartedAgainAfterItHasFailed)
{
    FakeSdCard::settings.present = false;
    BddTargetSdCard_Start();
    FakeSdCard::settings.present = true;
    CHECK_RESULT(BDD_TARGET_SD_CARD_OK, BddTargetSdCard_Start());
}

TEST(BddTargetSdCard, ForgetsTheCardWhenStopped)
{
    Started();
    BddTargetSdCard_Stop();
    CHECK_FALSE(BddTargetSdCard_IsStarted());
    LONGS_EQUAL(0, BddTargetSdCard_SectorCount());
}

TEST(BddTargetSdCard, ReadsASector)
{
    FakeSdCard::Store(A_SECTOR, SectorOfPattern(7));
    Started();
    CHECK_RESULT(BDD_TARGET_SD_CARD_OK, Read(A_SECTOR));
    CHECK_SECTORS_EQUAL(SectorOfPattern(7), buffer);
}

TEST(BddTargetSdCard, AddressesAHighCapacityCardBySector)
{
    Started();
    Read(A_SECTOR);
    LONGS_EQUAL(A_SECTOR, FakeSdCard::ArgumentOf(READ_SINGLE_BLOCK));
}

TEST(BddTargetSdCard, DoesNotSetTheBlockLengthOfAHighCapacityCard)
{
    Started();
    CHECK_FALSE(FakeSdCard::Received(SET_BLOCKLEN));
}

TEST(BddTargetSdCard, SetsAStandardCapacityCardToSectorsOf512Bytes)
{
    FakeSdCard::settings.highCapacity = false;
    Started();
    LONGS_EQUAL(512, FakeSdCard::ArgumentOf(SET_BLOCKLEN));
}

TEST(BddTargetSdCard, AddressesAStandardCapacityCardByByte)
{
    FakeSdCard::settings.highCapacity = false;
    FakeSdCard::Store(A_SECTOR, SectorOfPattern(9));
    Started();
    CHECK_RESULT(BDD_TARGET_SD_CARD_OK, Read(A_SECTOR));
    LONGS_EQUAL(A_SECTOR * 512, FakeSdCard::ArgumentOf(READ_SINGLE_BLOCK));
    CHECK_SECTORS_EQUAL(SectorOfPattern(9), buffer);
}

TEST(BddTargetSdCard, SaysTheCardRefusedARead)
{
    Started();
    FakeSdCard::settings.transferAnswer = ILLEGAL_COMMAND;
    CHECK_RESULT(BDD_TARGET_SD_CARD_REFUSED, Read(A_SECTOR));
}

TEST(BddTargetSdCard, TimesOutOnAReadTheCardSendsNothingFor)
{
    Started();
    FakeSdCard::settings.sendsNoData = true;
    CHECK_RESULT(BDD_TARGET_SD_CARD_TIMED_OUT, Read(A_SECTOR));
}

TEST(BddTargetSdCard, SaysTheDataIsBadWhenTheCardSendsAnErrorToken)
{
    Started();
    FakeSdCard::settings.readErrorToken = OUT_OF_RANGE_TOKEN;
    CHECK_RESULT(BDD_TARGET_SD_CARD_BAD_DATA, Read(A_SECTOR));
}

TEST(BddTargetSdCard, SaysTheDataIsBadWhenASectorFailsItsCrc)
{
    Started();
    FakeSdCard::settings.corruptsDataItSends = true;
    CHECK_RESULT(BDD_TARGET_SD_CARD_BAD_DATA, Read(A_SECTOR));
}

TEST(BddTargetSdCard, LeavesTheCardDeselectedAfterAReadThatFails)
{
    Started();
    FakeSdCard::settings.sendsNoData = true;
    Read(A_SECTOR);
    CHECK_FALSE(FakeSdCard::IsSelected());
}

TEST(BddTargetSdCard, WritesASector)
{
    Started();
    CHECK_RESULT(BDD_TARGET_SD_CARD_OK, Write(A_SECTOR, SectorOfPattern(42)));
    CHECK_TRUE(FakeSdCard::Holds(A_SECTOR));
    CHECK_SECTORS_EQUAL(SectorOfPattern(42), FakeSdCard::Stored(A_SECTOR));
}

TEST(BddTargetSdCard, ReadsBackWhatItWrote)
{
    Started();
    Write(A_SECTOR, SectorOfPattern(42));
    CHECK_RESULT(BDD_TARGET_SD_CARD_OK, Read(A_SECTOR));
    CHECK_SECTORS_EQUAL(SectorOfPattern(42), buffer);
}

TEST(BddTargetSdCard, WritesAStandardCapacityCardByByte)
{
    FakeSdCard::settings.highCapacity = false;
    Started();
    CHECK_RESULT(BDD_TARGET_SD_CARD_OK, Write(A_SECTOR, SectorOfPattern(42)));
    LONGS_EQUAL(A_SECTOR * 512, FakeSdCard::ArgumentOf(WRITE_BLOCK));
    CHECK_TRUE(FakeSdCard::Holds(A_SECTOR));
}

TEST(BddTargetSdCard, SaysTheCardRefusedAWrite)
{
    Started();
    FakeSdCard::settings.transferAnswer = ILLEGAL_COMMAND;
    CHECK_RESULT(BDD_TARGET_SD_CARD_REFUSED, Write(A_SECTOR, SectorOfPattern(42)));
    CHECK_FALSE(FakeSdCard::Holds(A_SECTOR));
}

TEST(BddTargetSdCard, SaysTheWriteFailedWhenTheCardDoesNotTakeTheSector)
{
    Started();
    FakeSdCard::settings.writeResponse = DATA_REJECTED_WRITE_ERROR;
    CHECK_RESULT(BDD_TARGET_SD_CARD_WRITE_FAILED, Write(A_SECTOR, SectorOfPattern(42)));
}

TEST(BddTargetSdCard, TimesOutOnACardThatStaysBusyAfterAWrite)
{
    Started();
    FakeSdCard::settings.staysBusyAfterWrite = true;
    CHECK_RESULT(BDD_TARGET_SD_CARD_TIMED_OUT, Write(A_SECTOR, SectorOfPattern(42)));
}

TEST(BddTargetSdCard, AsksTheCardForItsStatusAfterAWrite)
{
    Started();
    Write(A_SECTOR, SectorOfPattern(42));
    CHECK_TRUE(FakeSdCard::Received(SEND_STATUS));
}

TEST(BddTargetSdCard, SaysTheWriteFailedWhenTheCardsStatusSaysSo)
{
    Started();
    FakeSdCard::settings.statusAfterWrite = WRITE_PROTECT_VIOLATION;
    CHECK_RESULT(BDD_TARGET_SD_CARD_WRITE_FAILED, Write(A_SECTOR, SectorOfPattern(42)));
}

TEST(BddTargetSdCard, LeavesTheCardDeselectedAfterAWriteThatFails)
{
    Started();
    FakeSdCard::settings.writeResponse = DATA_REJECTED_WRITE_ERROR;
    Write(A_SECTOR, SectorOfPattern(42));
    CHECK_FALSE(FakeSdCard::IsSelected());
}
