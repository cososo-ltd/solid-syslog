#include "BddTargetSpiNorFlash.h"
#include "CppUTest/TestHarness.h"
#include "FakeSpiNorFlash.h"

#include <cstddef>
#include <cstdint>
#include <vector>

enum
{
    PAGE_PROGRAM = 0x02,
    READ_DATA = 0x03,
    READ_STATUS = 0x05,
    WRITE_ENABLE = 0x06,
    SECTOR_ERASE = 0x20,
    READ_JEDEC_ID = 0x9F,

    AN_ADDRESS = 0x012345,
    START_OF_A_PAGE = 0x042000,
    LAST_BYTE_OF_A_PAGE = 0x0420FF,
    A_SECTOR = 0x42,
    TWO_SECTORS = 2 * FakeSpiNorFlash::SECTOR_BYTES,
    THREE_PAGES_AND_A_LITTLE = (3 * FakeSpiNorFlash::PAGE_BYTES) + 10,
    SECTORS_IN_SIXTEEN_MEBIBYTES = 4096,
    SIXTEEN_MEBIBYTES = 16 * 1024 * 1024,
    CAPACITY_CODE_2_MEBIBYTES = 0x15,
    CAPACITY_CODE_8_MEBIBYTES = 0x17,
    CAPACITY_CODE_32_MEBIBYTES = 0x19,
    ANOTHER_MAKER = 0xC2,
    ANOTHER_FAMILY = 0x20,
    ALWAYS = 1000000,
    LONGER_THAN_ANY_WAIT = 60000
};

#define CHECK_RESULT(expected, actual) LONGS_EQUAL((expected), (actual))

/* A macro, so a part that does not start is reported at the test's own line. */
#define CHECK_FLASH_STARTED() CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OK, BddTargetSpiNorFlash_Start())

#define CHECK_BYTES_EQUAL(expected, actual) MEMCMP_EQUAL((expected).data(), (actual).data(), (expected).size())

static std::vector<uint8_t> BytesOfPattern(uint8_t first, size_t count)
{
    std::vector<uint8_t> bytes(count);
    uint8_t next = first;
    for (uint8_t& byte : bytes)
    {
        byte = next;
        next++;
    }
    return bytes;
}

// clang-format off
TEST_GROUP(BddTargetSpiNorFlash)
{
    std::vector<uint8_t> buffer;

    void setup() override
    {
        FakeSpiNorFlash::Reset();
        BddTargetSpiNorFlash_Stop();
    }

    void teardown() override
    {
        /* The fake holds what it was sent in memory the leak check counts. */
        FakeSpiNorFlash::Release();
        buffer = std::vector<uint8_t>();
    }

    BddTargetSpiNorFlashResult Read(uint32_t address, size_t count)
    {
        buffer.assign(count, 0);
        return BddTargetSpiNorFlash_Read(address, buffer.data(), count);
    }

    static BddTargetSpiNorFlashResult Program(uint32_t address, const std::vector<uint8_t>& bytes)
    {
        return BddTargetSpiNorFlash_Program(address, bytes.data(), bytes.size());
    }
};

// clang-format on

TEST(BddTargetSpiNorFlash, IsNotStartedUntilItIsStarted)
{
    CHECK_FALSE(BddTargetSpiNorFlash_IsStarted());
}

TEST(BddTargetSpiNorFlash, HasNoCapacityUntilItIsStarted)
{
    LONGS_EQUAL(0, BddTargetSpiNorFlash_Capacity());
}

TEST(BddTargetSpiNorFlash, StartsAPartThatIdentifiesItself)
{
    CHECK_FLASH_STARTED();
    CHECK_TRUE(BddTargetSpiNorFlash_IsStarted());
}

TEST(BddTargetSpiNorFlash, AsksThePartToIdentifyItself)
{
    CHECK_FLASH_STARTED();
    LONGS_EQUAL(1, FakeSpiNorFlash::CountOf(READ_JEDEC_ID));
}

TEST(BddTargetSpiNorFlash, TakesTheBusBeforeItsFirstCommand)
{
    CHECK_FLASH_STARTED();
    CHECK_TRUE(FakeSpiNorFlash::BusWasTakenBeforeFirstCommand());
}

TEST(BddTargetSpiNorFlash, LeavesThePartDeselectedOnceStarted)
{
    CHECK_FLASH_STARTED();
    CHECK_FALSE(FakeSpiNorFlash::IsSelected());
}

TEST(BddTargetSpiNorFlash, HasTheCapacityThePartIdentifiesWith)
{
    CHECK_FLASH_STARTED();
    LONGS_EQUAL(SIXTEEN_MEBIBYTES, BddTargetSpiNorFlash_Capacity());
}

TEST(BddTargetSpiNorFlash, HasTheCapacityOfASmallerPart)
{
    FakeSpiNorFlash::settings.jedecId[2] = CAPACITY_CODE_8_MEBIBYTES;
    CHECK_FLASH_STARTED();
    LONGS_EQUAL(SIXTEEN_MEBIBYTES / 2, BddTargetSpiNorFlash_Capacity());
}

TEST(BddTargetSpiNorFlash, DoesNotStartWithNoPartOnTheBus)
{
    FakeSpiNorFlash::settings.present = false;
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_NO_ANSWER, BddTargetSpiNorFlash_Start());
    CHECK_FALSE(BddTargetSpiNorFlash_IsStarted());
}

TEST(BddTargetSpiNorFlash, DoesNotStartWithTheLineHeldLow)
{
    FakeSpiNorFlash::settings.lineHeldLow = true;
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_NO_ANSWER, BddTargetSpiNorFlash_Start());
}

TEST(BddTargetSpiNorFlash, RefusesAPartOfAnotherMaker)
{
    FakeSpiNorFlash::settings.jedecId[0] = ANOTHER_MAKER;
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_UNSUPPORTED, BddTargetSpiNorFlash_Start());
    CHECK_FALSE(BddTargetSpiNorFlash_IsStarted());
}

TEST(BddTargetSpiNorFlash, RefusesAPartThreeBytesOfAddressDoNotReachTheEndOf)
{
    FakeSpiNorFlash::settings.jedecId[2] = CAPACITY_CODE_32_MEBIBYTES;
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_UNSUPPORTED, BddTargetSpiNorFlash_Start());
}

TEST(BddTargetSpiNorFlash, WaitsForAPartThatIsBusyBeforeAskingItAnything)
{
    FakeSpiNorFlash::settings.busyStatusReadsAtStart = 5;
    CHECK_FLASH_STARTED();
    LONGS_EQUAL(0, FakeSpiNorFlash::CommandsWhileBusy());
}

TEST(BddTargetSpiNorFlash, GivesUpOnAPartThatStaysBusy)
{
    FakeSpiNorFlash::settings.staysBusy = true;
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_TIMED_OUT, BddTargetSpiNorFlash_Start());
    CHECK_FALSE(BddTargetSpiNorFlash_IsStarted());
}

TEST(BddTargetSpiNorFlash, AsksOnceMoreAfterTheTimeAllowedHasPassed)
{
    /* A task switched out for longer than the wait comes back to a part that
     * finished while it was away. */
    FakeSpiNorFlash::settings.busyStatusReadsAtStart = 1;
    FakeSpiNorFlash::settings.millisecondsPerReading = LONGER_THAN_ANY_WAIT;
    CHECK_FLASH_STARTED();
}

TEST(BddTargetSpiNorFlash, IsNotStartedOnceStopped)
{
    CHECK_FLASH_STARTED();
    BddTargetSpiNorFlash_Stop();
    CHECK_FALSE(BddTargetSpiNorFlash_IsStarted());
    LONGS_EQUAL(0, BddTargetSpiNorFlash_Capacity());
}

TEST(BddTargetSpiNorFlash, IsNotStartedAfterAStartThatFails)
{
    CHECK_FLASH_STARTED();
    FakeSpiNorFlash::settings.present = false;
    (void) BddTargetSpiNorFlash_Start();
    CHECK_FALSE(BddTargetSpiNorFlash_IsStarted());
}

TEST(BddTargetSpiNorFlash, ReadsNothingBeforeItIsStarted)
{
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_NOT_STARTED, Read(AN_ADDRESS, 4));
    LONGS_EQUAL(0, FakeSpiNorFlash::Exchanges());
}

TEST(BddTargetSpiNorFlash, ReadsTheBytesAtAnAddress)
{
    const std::vector<uint8_t> held = {0x11, 0x22, 0x33, 0x44, 0x55};
    FakeSpiNorFlash::Store(AN_ADDRESS, held);
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OK, Read(AN_ADDRESS, held.size()));
    CHECK_BYTES_EQUAL(held, buffer);
}

TEST(BddTargetSpiNorFlash, ReadsAcrossSectorsWithOneCommand)
{
    const std::vector<uint8_t> held = BytesOfPattern(3, TWO_SECTORS);
    FakeSpiNorFlash::Store(LAST_BYTE_OF_A_PAGE, held);
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OK, Read(LAST_BYTE_OF_A_PAGE, held.size()));
    CHECK_BYTES_EQUAL(held, buffer);
    LONGS_EQUAL(1, FakeSpiNorFlash::CountOf(READ_DATA));
}

TEST(BddTargetSpiNorFlash, LeavesThePartDeselectedAfterARead)
{
    CHECK_FLASH_STARTED();
    (void) Read(AN_ADDRESS, 4);
    CHECK_FALSE(FakeSpiNorFlash::IsSelected());
}

TEST(BddTargetSpiNorFlash, ReadsThePartsLastByte)
{
    FakeSpiNorFlash::Store(SIXTEEN_MEBIBYTES - 1, {0x5A});
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OK, Read(SIXTEEN_MEBIBYTES - 1, 1));
    BYTES_EQUAL(0x5A, buffer[0]);
}

TEST(BddTargetSpiNorFlash, RefusesAReadThatRunsPastTheEndOfThePart)
{
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OUT_OF_RANGE, Read(SIXTEEN_MEBIBYTES - 1, 2));
    LONGS_EQUAL(0, FakeSpiNorFlash::CountOf(READ_DATA));
}

TEST(BddTargetSpiNorFlash, RefusesAReadThatBeginsPastTheEndOfThePart)
{
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OUT_OF_RANGE, Read(UINT32_MAX, 2));
}

TEST(BddTargetSpiNorFlash, ProgramsNothingBeforeItIsStarted)
{
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_NOT_STARTED, Program(AN_ADDRESS, {0x00}));
    LONGS_EQUAL(0, FakeSpiNorFlash::Exchanges());
}

TEST(BddTargetSpiNorFlash, ProgramsBytesAtAnAddress)
{
    const std::vector<uint8_t> bytes = {0x12, 0x34, 0x56};
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OK, Program(AN_ADDRESS, bytes));
    CHECK_BYTES_EQUAL(bytes, FakeSpiNorFlash::Stored(AN_ADDRESS, bytes.size()));
}

TEST(BddTargetSpiNorFlash, ProgramsAWholePageWithOneCommand)
{
    const std::vector<uint8_t> bytes = BytesOfPattern(1, FakeSpiNorFlash::PAGE_BYTES);
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OK, Program(START_OF_A_PAGE, bytes));
    CHECK_BYTES_EQUAL(bytes, FakeSpiNorFlash::Stored(START_OF_A_PAGE, bytes.size()));
    LONGS_EQUAL(1, FakeSpiNorFlash::Programs().size());
}

TEST(BddTargetSpiNorFlash, IsNoLongerBusyWhenAProgramReturns)
{
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OK, Program(AN_ADDRESS, {0x00}));
    CHECK_FALSE(FakeSpiNorFlash::IsBusy());
}

TEST(BddTargetSpiNorFlash, SplitsAProgramWhereItCrossesIntoTheNextPage)
{
    const std::vector<uint8_t> bytes = {0x01, 0x02, 0x03};
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OK, Program(LAST_BYTE_OF_A_PAGE, bytes));
    CHECK_BYTES_EQUAL(bytes, FakeSpiNorFlash::Stored(LAST_BYTE_OF_A_PAGE, bytes.size()));
    LONGS_EQUAL(2, FakeSpiNorFlash::Programs().size());
    LONGS_EQUAL(LAST_BYTE_OF_A_PAGE, FakeSpiNorFlash::Programs()[0].address);
    LONGS_EQUAL(1, FakeSpiNorFlash::Programs()[0].length);
    LONGS_EQUAL(LAST_BYTE_OF_A_PAGE + 1, FakeSpiNorFlash::Programs()[1].address);
    LONGS_EQUAL(2, FakeSpiNorFlash::Programs()[1].length);
}

TEST(BddTargetSpiNorFlash, ProgramsASpanOfManyPages)
{
    const std::vector<uint8_t> bytes = BytesOfPattern(7, THREE_PAGES_AND_A_LITTLE);
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OK, Program(LAST_BYTE_OF_A_PAGE, bytes));
    CHECK_BYTES_EQUAL(bytes, FakeSpiNorFlash::Stored(LAST_BYTE_OF_A_PAGE, bytes.size()));
    LONGS_EQUAL(5, FakeSpiNorFlash::Programs().size());
}

TEST(BddTargetSpiNorFlash, EnablesWritingBeforeEveryPageItPrograms)
{
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OK, Program(LAST_BYTE_OF_A_PAGE, {0x01, 0x02}));
    LONGS_EQUAL(2, FakeSpiNorFlash::CountOf(WRITE_ENABLE));
}

TEST(BddTargetSpiNorFlash, WaitsForOnePageToFinishBeforeItProgramsTheNext)
{
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OK, Program(LAST_BYTE_OF_A_PAGE, {0x01, 0x02}));
    LONGS_EQUAL(0, FakeSpiNorFlash::CommandsWhileBusy());
}

TEST(BddTargetSpiNorFlash, ProgramsNothingWhenGivenNothing)
{
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OK, Program(AN_ADDRESS, {}));
    LONGS_EQUAL(0, FakeSpiNorFlash::CountOf(WRITE_ENABLE));
}

TEST(BddTargetSpiNorFlash, SendsNoProgramToAPartThatDidNotEnableWriting)
{
    CHECK_FLASH_STARTED();
    FakeSpiNorFlash::settings.ignoresWriteEnable = true;
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_WRITE_REFUSED, Program(AN_ADDRESS, {0x00}));
    LONGS_EQUAL(0, FakeSpiNorFlash::CountOf(PAGE_PROGRAM));
}

TEST(BddTargetSpiNorFlash, GivesUpOnAProgramThePartNeverFinishes)
{
    CHECK_FLASH_STARTED();
    FakeSpiNorFlash::settings.busyStatusReads = ALWAYS;
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_TIMED_OUT, Program(AN_ADDRESS, {0x00}));
}

TEST(BddTargetSpiNorFlash, StopsAtThePageThatFails)
{
    CHECK_FLASH_STARTED();
    FakeSpiNorFlash::settings.busyStatusReads = ALWAYS;
    (void) Program(LAST_BYTE_OF_A_PAGE, {0x01, 0x02});
    LONGS_EQUAL(1, FakeSpiNorFlash::Programs().size());
}

TEST(BddTargetSpiNorFlash, RefusesAProgramThatRunsPastTheEndOfThePart)
{
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OUT_OF_RANGE, Program(SIXTEEN_MEBIBYTES - 1, {0x00, 0x00}));
    LONGS_EQUAL(0, FakeSpiNorFlash::CountOf(WRITE_ENABLE));
}

TEST(BddTargetSpiNorFlash, ErasesNothingBeforeItIsStarted)
{
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_NOT_STARTED, BddTargetSpiNorFlash_EraseSector(A_SECTOR));
    LONGS_EQUAL(0, FakeSpiNorFlash::Exchanges());
}

TEST(BddTargetSpiNorFlash, ErasesASector)
{
    FakeSpiNorFlash::Store(A_SECTOR * FakeSpiNorFlash::SECTOR_BYTES, BytesOfPattern(0, FakeSpiNorFlash::SECTOR_BYTES));
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OK, BddTargetSpiNorFlash_EraseSector(A_SECTOR));
    CHECK_BYTES_EQUAL(
        std::vector<uint8_t>(FakeSpiNorFlash::SECTOR_BYTES, 0xFF),
        FakeSpiNorFlash::Stored(A_SECTOR * FakeSpiNorFlash::SECTOR_BYTES, FakeSpiNorFlash::SECTOR_BYTES)
    );
}

TEST(BddTargetSpiNorFlash, ErasesOneSectorAndNoOther)
{
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OK, BddTargetSpiNorFlash_EraseSector(A_SECTOR));
    LONGS_EQUAL(1, FakeSpiNorFlash::Erases().size());
    LONGS_EQUAL(A_SECTOR * FakeSpiNorFlash::SECTOR_BYTES, FakeSpiNorFlash::Erases()[0]);
}

TEST(BddTargetSpiNorFlash, IsNoLongerBusyWhenAnEraseReturns)
{
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OK, BddTargetSpiNorFlash_EraseSector(A_SECTOR));
    CHECK_FALSE(FakeSpiNorFlash::IsBusy());
}

TEST(BddTargetSpiNorFlash, SendsNoEraseToAPartThatDidNotEnableWriting)
{
    CHECK_FLASH_STARTED();
    FakeSpiNorFlash::settings.ignoresWriteEnable = true;
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_WRITE_REFUSED, BddTargetSpiNorFlash_EraseSector(A_SECTOR));
    LONGS_EQUAL(0, FakeSpiNorFlash::CountOf(SECTOR_ERASE));
}

TEST(BddTargetSpiNorFlash, GivesUpOnAnEraseThePartNeverFinishes)
{
    CHECK_FLASH_STARTED();
    FakeSpiNorFlash::settings.busyStatusReads = ALWAYS;
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_TIMED_OUT, BddTargetSpiNorFlash_EraseSector(A_SECTOR));
}

TEST(BddTargetSpiNorFlash, ErasesTheLastSectorOfThePart)
{
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OK, BddTargetSpiNorFlash_EraseSector(SECTORS_IN_SIXTEEN_MEBIBYTES - 1));
}

TEST(BddTargetSpiNorFlash, RefusesToEraseASectorPastTheEndOfThePart)
{
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OUT_OF_RANGE, BddTargetSpiNorFlash_EraseSector(SECTORS_IN_SIXTEEN_MEBIBYTES));
    LONGS_EQUAL(0, FakeSpiNorFlash::CountOf(WRITE_ENABLE));
}

TEST(BddTargetSpiNorFlash, SaysThereIsNoAnswerWhenThePartGoesQuietDuringAnErase)
{
    CHECK_FLASH_STARTED();
    FakeSpiNorFlash::settings.present = false;
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_NO_ANSWER, BddTargetSpiNorFlash_EraseSector(A_SECTOR));
}

TEST(BddTargetSpiNorFlash, SaysThereIsNoAnswerWhenThePartGoesQuietDuringARead)
{
    CHECK_FLASH_STARTED();
    FakeSpiNorFlash::settings.present = false;
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_NO_ANSWER, Read(AN_ADDRESS, 4));
}

TEST(BddTargetSpiNorFlash, ReadsErasedBytesFromAPartThatAnswers)
{
    CHECK_FLASH_STARTED();
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_OK, Read(AN_ADDRESS, 4));
    CHECK_BYTES_EQUAL(std::vector<uint8_t>(4, 0xFF), buffer);
}

TEST(BddTargetSpiNorFlash, HasWordsForEveryResult)
{
    STRCMP_EQUAL("ok", BddTargetSpiNorFlash_ResultText(BDD_TARGET_SPI_NOR_FLASH_OK));
    STRCMP_EQUAL("not started", BddTargetSpiNorFlash_ResultText(BDD_TARGET_SPI_NOR_FLASH_NOT_STARTED));
    STRCMP_EQUAL("out of range", BddTargetSpiNorFlash_ResultText(BDD_TARGET_SPI_NOR_FLASH_OUT_OF_RANGE));
    STRCMP_EQUAL("no answer", BddTargetSpiNorFlash_ResultText(BDD_TARGET_SPI_NOR_FLASH_NO_ANSWER));
    STRCMP_EQUAL("not a part this drives", BddTargetSpiNorFlash_ResultText(BDD_TARGET_SPI_NOR_FLASH_UNSUPPORTED));
    STRCMP_EQUAL("timed out", BddTargetSpiNorFlash_ResultText(BDD_TARGET_SPI_NOR_FLASH_TIMED_OUT));
    STRCMP_EQUAL("would not enable writing", BddTargetSpiNorFlash_ResultText(BDD_TARGET_SPI_NOR_FLASH_WRITE_REFUSED));
}

TEST(BddTargetSpiNorFlash, RefusesAPartOfTheMakersThatIsNotOfTheFamily)
{
    FakeSpiNorFlash::settings.jedecId = {0xEF, ANOTHER_FAMILY, CAPACITY_CODE_2_MEBIBYTES};
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_UNSUPPORTED, BddTargetSpiNorFlash_Start());
    CHECK_FALSE(BddTargetSpiNorFlash_IsStarted());
}

TEST(BddTargetSpiNorFlash, IsNotStartedAfterAProgramThePartNeverFinishes)
{
    CHECK_FLASH_STARTED();
    FakeSpiNorFlash::settings.busyStatusReads = ALWAYS;
    (void) Program(AN_ADDRESS, {0x00});
    CHECK_FALSE(BddTargetSpiNorFlash_IsStarted());
}

TEST(BddTargetSpiNorFlash, IsNotStartedAfterAnEraseThePartNeverFinishes)
{
    CHECK_FLASH_STARTED();
    FakeSpiNorFlash::settings.busyStatusReads = ALWAYS;
    (void) BddTargetSpiNorFlash_EraseSector(A_SECTOR);
    CHECK_FALSE(BddTargetSpiNorFlash_IsStarted());
}

TEST(BddTargetSpiNorFlash, ReadsNothingFromAPartLeftBusyByAProgramItNeverFinished)
{
    CHECK_FLASH_STARTED();
    FakeSpiNorFlash::settings.busyStatusReads = ALWAYS;
    (void) Program(AN_ADDRESS, {0x00});
    CHECK_RESULT(BDD_TARGET_SPI_NOR_FLASH_NOT_STARTED, Read(AN_ADDRESS, 4));
    LONGS_EQUAL(0, FakeSpiNorFlash::CountOf(READ_DATA));
}

TEST(BddTargetSpiNorFlash, StaysStartedWhenThePartWillNotEnableWriting)
{
    CHECK_FLASH_STARTED();
    FakeSpiNorFlash::settings.ignoresWriteEnable = true;
    (void) Program(AN_ADDRESS, {0x00});
    CHECK_TRUE(BddTargetSpiNorFlash_IsStarted());
}
