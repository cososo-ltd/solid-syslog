#include "SolidSyslogSpiNorFlashBus.h"
#include "SpiNorFlashPartFake.h"
#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

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
    BLOCK_ERASE = 0xD8,
    READ_SFDP = 0x5A,
    READ_JEDEC_ID = 0x9F,

    STATUS_BUSY = 0x01,
    STATUS_WRITE_ENABLED = 0x02,
    STATUS_PROTECTION = 0x7C,

    AN_ADDRESS = 0x012345,
    START_OF_A_SECTOR = 0x042000,
    LAST_BYTE_OF_A_PAGE = 0x0420FF,
    START_OF_THAT_PAGE = 0x042000,
    SECTOR_BYTES = 4096,
    BLOCK_BYTES = 65536,
    START_OF_A_BLOCK = 0x040000,
    SFDP_TABLE_ADDRESS = 0x80,
    ERASE_MILLISECONDS = 50,

    EIGHT_MEBIBYTES = 8 * 1024 * 1024,
    NOT_READ = 0xA5
};

/* Two readings before the count of milliseconds wraps to zero. */
static const uint32_t JUST_BEFORE_THE_WRAP = UINT32_MAX - 1U;

using Bytes = std::vector<uint8_t>;

// clang-format off
TEST_GROUP(SpiNorFlashPartFake)
{
    SolidSyslogSpiNorFlashBus* bus = nullptr;

    void setup() override
    {
        SpiNorFlashPartFake::Reset();
        bus = SpiNorFlashPartFake::Bus();
    }

    void teardown() override
    {
        /* The fake holds what it was sent in memory the leak check counts. */
        SpiNorFlashPartFake::Release();
    }

    /* One command: what is written, then how many bytes are read back. The
     * bytes start as something no test expects, so a read that left them
     * alone shows. */
    [[nodiscard]] Bytes Command(const Bytes& out, size_t readCount = 0) const
    {
        Bytes in(readCount, NOT_READ);
        bus->Select(bus);
        (void) bus->Write(bus, out.data(), out.size());
        (void) bus->Read(bus, in.data(), in.size());
        bus->Deselect(bus);
        return in;
    }

    [[nodiscard]] uint8_t Status() const
    {
        return Command({READ_STATUS}, 1)[0];
    }

    static Bytes WithAddress(uint8_t command, uint32_t address, const Bytes& data = {})
    {
        Bytes frame = {command, static_cast<uint8_t>(address >> 16U), static_cast<uint8_t>(address >> 8U),
                       static_cast<uint8_t>(address)};
        frame.insert(frame.end(), data.begin(), data.end());
        return frame;
    }

    void Program(uint32_t address, const Bytes& data) const
    {
        (void) Command({WRITE_ENABLE});
        (void) Command(WithAddress(PAGE_PROGRAM, address, data));
    }

    void Erase(uint8_t command, uint32_t address) const
    {
        (void) Command({WRITE_ENABLE});
        (void) Command(WithAddress(command, address));
    }

    [[nodiscard]] Bytes Sfdp(uint32_t address, size_t count) const
    {
        return Command(WithAddress(READ_SFDP, address, {0x00}), count);
    }

    void LetTimePass(uint32_t milliseconds) const
    {
        for (uint32_t i = 0; i < milliseconds; i++)
        {
            (void) bus->Milliseconds(bus);
        }
    }
};

// clang-format on

TEST(SpiNorFlashPartFake, ReadsAnIdleLineWhenNotSelected)
{
    Bytes in(2, 0x00);
    CHECK_TRUE(bus->Read(bus, in.data(), in.size()));
    CHECK_BYTES_EQUAL(Bytes({0xFF, 0xFF}), in);
}

TEST(SpiNorFlashPartFake, CountsTransfersMadeWhileNotSelected)
{
    uint8_t byte = READ_STATUS;
    (void) bus->Write(bus, &byte, 1);
    (void) bus->Read(bus, &byte, 1);
    LONGS_EQUAL(2, SpiNorFlashPartFake::TransfersWhileDeselected());
}

TEST(SpiNorFlashPartFake, CountsNoTransferMadeWhileSelected)
{
    (void) Status();
    LONGS_EQUAL(0, SpiNorFlashPartFake::TransfersWhileDeselected());
}

TEST(SpiNorFlashPartFake, IsSelectedBetweenSelectAndDeselect)
{
    CHECK_FALSE(SpiNorFlashPartFake::IsSelected());
    bus->Select(bus);
    CHECK_TRUE(SpiNorFlashPartFake::IsSelected());
    bus->Deselect(bus);
    CHECK_FALSE(SpiNorFlashPartFake::IsSelected());
}

TEST(SpiNorFlashPartFake, IdentifiesItself)
{
    CHECK_BYTES_EQUAL(Bytes({0xEF, 0x40, 0x18}), Command({READ_JEDEC_ID}, 3));
}

TEST(SpiNorFlashPartFake, ReadsAnIdleLineWithNoPartOnTheBus)
{
    SpiNorFlashPartFake::settings.present = false;
    CHECK_BYTES_EQUAL(Bytes({0xFF, 0xFF, 0xFF}), Command({READ_JEDEC_ID}, 3));
}

TEST(SpiNorFlashPartFake, ReadsZerosWithTheLineHeldLow)
{
    /* Asked of the bus itself, so that the read is seen to answer true. */
    SpiNorFlashPartFake::settings.lineHeldLow = true;
    Bytes in(3, NOT_READ);
    bus->Select(bus);
    CHECK_TRUE(bus->Read(bus, in.data(), in.size()));
    bus->Deselect(bus);
    CHECK_BYTES_EQUAL(Bytes({0x00, 0x00, 0x00}), in);
}

TEST(SpiNorFlashPartFake, CountsEveryCallOfItsBus)
{
    uint8_t byte = READ_STATUS;
    bus->Select(bus);
    (void) bus->Write(bus, &byte, 1);
    (void) bus->Read(bus, &byte, 1);
    bus->Deselect(bus);
    (void) bus->Milliseconds(bus);
    LONGS_EQUAL(5, SpiNorFlashPartFake::BusCalls());
}

TEST(SpiNorFlashPartFake, HasCountedNoCallOfItsBusBeforeOneIsMade)
{
    LONGS_EQUAL(0, SpiNorFlashPartFake::BusCalls());
}

TEST(SpiNorFlashPartFake, LeavesWhileBusyUnfilledUntilATestFillsIt)
{
    CHECK_TRUE(bus->WhileBusy == nullptr);
    SpiNorFlashPartFake::FillWhileBusy();
    CHECK_TRUE(bus->WhileBusy != nullptr);
}

TEST(SpiNorFlashPartFake, LeavesWhileBusyUnfilledAgainAfterAReset)
{
    SpiNorFlashPartFake::FillWhileBusy();
    SpiNorFlashPartFake::Reset();
    CHECK_TRUE(bus->WhileBusy == nullptr);
}

TEST(SpiNorFlashPartFake, CountsCallsOfWhileBusyAmongThoseOfItsBus)
{
    SpiNorFlashPartFake::FillWhileBusy();
    bus->WhileBusy(bus);
    bus->WhileBusy(bus);
    LONGS_EQUAL(2, SpiNorFlashPartFake::WhileBusyCalls());
    LONGS_EQUAL(2, SpiNorFlashPartFake::BusCalls());
}

TEST(SpiNorFlashPartFake, PassesNoTimeInWhileBusyUntilToldTo)
{
    SpiNorFlashPartFake::FillWhileBusy();
    bus->WhileBusy(bus);
    LONGS_EQUAL(0, SpiNorFlashPartFake::Milliseconds());
}

TEST(SpiNorFlashPartFake, MovesTheTimeOnInWhileBusyByWhatItIsSet)
{
    SpiNorFlashPartFake::FillWhileBusy();
    SpiNorFlashPartFake::settings.millisecondsPerWhileBusy = 7;
    bus->WhileBusy(bus);
    LONGS_EQUAL(7, SpiNorFlashPartFake::Milliseconds());
}

TEST(SpiNorFlashPartFake, SaysWhenWhileBusyWasGivenTheBus)
{
    SpiNorFlashPartFake::FillWhileBusy();
    bus->WhileBusy(bus);
    CHECK_TRUE(SpiNorFlashPartFake::WhileBusyWasGivenTheBus());
}

TEST(SpiNorFlashPartFake, SaysWhenWhileBusyWasGivenSomethingElse)
{
    SpiNorFlashPartFake::FillWhileBusy();
    SolidSyslogSpiNorFlashBus another = *bus;
    bus->WhileBusy(&another);
    CHECK_FALSE(SpiNorFlashPartFake::WhileBusyWasGivenTheBus());
}

TEST(SpiNorFlashPartFake, CountsCallsOfWhileBusyMadeWhileThePartIsSelected)
{
    SpiNorFlashPartFake::FillWhileBusy();
    bus->WhileBusy(bus);
    bus->Select(bus);
    bus->WhileBusy(bus);
    bus->Deselect(bus);
    LONGS_EQUAL(1, SpiNorFlashPartFake::WhileBusyCallsWhileSelected());
}

TEST(SpiNorFlashPartFake, RecordsTheCommandsItTakes)
{
    (void) Command({READ_JEDEC_ID}, 3);
    (void) Status();
    CHECK_BYTES_EQUAL(Bytes({READ_JEDEC_ID, READ_STATUS}), SpiNorFlashPartFake::Commands());
    LONGS_EQUAL(1, SpiNorFlashPartFake::CountOf(READ_STATUS));
}

TEST(SpiNorFlashPartFake, HasAStatusOfZeroWhenIdle)
{
    BYTES_EQUAL(0x00, Status());
}

TEST(SpiNorFlashPartFake, ShowsThatWritingIsEnabled)
{
    (void) Command({WRITE_ENABLE});
    BYTES_EQUAL(STATUS_WRITE_ENABLED, Status());
}

TEST(SpiNorFlashPartFake, DoesNotEnableWritingWhenToldToIgnoreIt)
{
    SpiNorFlashPartFake::settings.ignoresWriteEnable = true;
    (void) Command({WRITE_ENABLE});
    BYTES_EQUAL(0x00, Status());
}

TEST(SpiNorFlashPartFake, ShowsTheProtectionBitsItIsGiven)
{
    SpiNorFlashPartFake::settings.status = STATUS_PROTECTION;
    BYTES_EQUAL(STATUS_PROTECTION, Status());
}

TEST(SpiNorFlashPartFake, SetsBusyAndWriteEnabledItselfWhateverItIsGiven)
{
    SpiNorFlashPartFake::settings.status = 0xFF;
    BYTES_EQUAL(0xFC, Status());
}

TEST(SpiNorFlashPartFake, AnswersTheStartOfItsSfdpHeader)
{
    CHECK_BYTES_EQUAL(Bytes({'S', 'F', 'D', 'P'}), Sfdp(0, 4));
}

TEST(SpiNorFlashPartFake, AnswersItsSfdpTableFromTheAddressAsked)
{
    CHECK_BYTES_EQUAL(Bytes({0xE5, 0x20, 0xF9, 0xFF}), Sfdp(SFDP_TABLE_ADDRESS, 4));
}

TEST(SpiNorFlashPartFake, ReadsAllOnesBetweenTheSfdpHeaderAndTheTable)
{
    CHECK_BYTES_EQUAL(Bytes({0xFF, 0xFF}), Sfdp(0x10, 2));
}

TEST(SpiNorFlashPartFake, RepeatsItsSfdpEvery256Bytes)
{
    CHECK_BYTES_EQUAL(Bytes({'S', 'F', 'D', 'P'}), Sfdp(0x100, 4));
}

TEST(SpiNorFlashPartFake, AnswersNoSfdpBeforeTheIgnoredByteArrives)
{
    CHECK_BYTES_EQUAL(Bytes({0xFF, 0xFF}), Command(WithAddress(READ_SFDP, 0), 2));
}

TEST(SpiNorFlashPartFake, AnswersTheSfdpATestGivesIt)
{
    SpiNorFlashPartFake::settings.sfdp[0] = 0x00;
    BYTES_EQUAL(0x00, Sfdp(0, 1)[0]);
}

TEST(SpiNorFlashPartFake, LeavesTheLineAloneWhenItHasNoSfdp)
{
    SpiNorFlashPartFake::settings.answersSfdp = false;
    CHECK_BYTES_EQUAL(Bytes({0xFF, 0xFF, 0xFF, 0xFF}), Sfdp(0, 4));
}

TEST(SpiNorFlashPartFake, RecordsWhatIsReadOfItsSfdp)
{
    (void) Sfdp(0x10, 8);
    LONGS_EQUAL(1, SpiNorFlashPartFake::SfdpReads().size());
    LONGS_EQUAL(0x10, SpiNorFlashPartFake::SfdpReads()[0].address);
    LONGS_EQUAL(8, SpiNorFlashPartFake::SfdpReads()[0].length);
}

TEST(SpiNorFlashPartFake, RecordsNoReadOfItsSfdpForAnotherCommand)
{
    (void) Command(WithAddress(READ_DATA, AN_ADDRESS), 2);
    LONGS_EQUAL(0, SpiNorFlashPartFake::SfdpReads().size());
}

TEST(SpiNorFlashPartFake, ReadsErasedBytesFromABlankPart)
{
    CHECK_BYTES_EQUAL(Bytes({0xFF, 0xFF}), Command(WithAddress(READ_DATA, AN_ADDRESS), 2));
}

TEST(SpiNorFlashPartFake, ReadsWhatIsStoredAtAnAddress)
{
    SpiNorFlashPartFake::Store(AN_ADDRESS, {0x11, 0x22, 0x33});
    CHECK_BYTES_EQUAL(Bytes({0x11, 0x22, 0x33}), Command(WithAddress(READ_DATA, AN_ADDRESS), 3));
}

TEST(SpiNorFlashPartFake, CarriesOnAReadAcrossTwoCallsOfTheBus)
{
    SpiNorFlashPartFake::Store(AN_ADDRESS, {0x11, 0x22});
    const Bytes frame = WithAddress(READ_DATA, AN_ADDRESS);
    uint8_t first = 0;
    uint8_t second = 0;
    bus->Select(bus);
    (void) bus->Write(bus, frame.data(), frame.size());
    (void) bus->Read(bus, &first, 1);
    (void) bus->Read(bus, &second, 1);
    bus->Deselect(bus);
    BYTES_EQUAL(0x11, first);
    BYTES_EQUAL(0x22, second);
}

TEST(SpiNorFlashPartFake, ProgramsBytesIntoErasedCells)
{
    Program(AN_ADDRESS, {0x12, 0x34});
    CHECK_BYTES_EQUAL(Bytes({0x12, 0x34}), SpiNorFlashPartFake::Stored(AN_ADDRESS, 2));
}

TEST(SpiNorFlashPartFake, RecordsTheProgramsItCarriesOut)
{
    Program(AN_ADDRESS, {0x12, 0x34});
    LONGS_EQUAL(1, SpiNorFlashPartFake::Programs().size());
    LONGS_EQUAL(AN_ADDRESS, SpiNorFlashPartFake::Programs()[0].address);
    LONGS_EQUAL(2, SpiNorFlashPartFake::Programs()[0].length);
}

TEST(SpiNorFlashPartFake, RecordsAProgramAtTheAddressItWasSent)
{
    SpiNorFlashPartFake::settings.capacityBytes = EIGHT_MEBIBYTES;
    Program(EIGHT_MEBIBYTES + AN_ADDRESS, {0x12});
    LONGS_EQUAL(EIGHT_MEBIBYTES + AN_ADDRESS, SpiNorFlashPartFake::Programs()[0].address);
}

TEST(SpiNorFlashPartFake, ProgramsAnAddressPastItsEndAsOneWrappedToItsStart)
{
    SpiNorFlashPartFake::settings.capacityBytes = EIGHT_MEBIBYTES;
    Program(EIGHT_MEBIBYTES + AN_ADDRESS, {0x12});
    BYTES_EQUAL(0x12, SpiNorFlashPartFake::Stored(AN_ADDRESS, 1)[0]);
}

TEST(SpiNorFlashPartFake, ClearsBitsAndSetsNoneWhenItPrograms)
{
    SpiNorFlashPartFake::Store(AN_ADDRESS, {0xF0});
    Program(AN_ADDRESS, {0x3C});
    BYTES_EQUAL(0x30, SpiNorFlashPartFake::Stored(AN_ADDRESS, 1)[0]);
}

TEST(SpiNorFlashPartFake, ProgramsNothingUnlessWritingWasEnabled)
{
    (void) Command(WithAddress(PAGE_PROGRAM, AN_ADDRESS, {0x00}));
    BYTES_EQUAL(0xFF, SpiNorFlashPartFake::Stored(AN_ADDRESS, 1)[0]);
    LONGS_EQUAL(0, SpiNorFlashPartFake::Programs().size());
}

TEST(SpiNorFlashPartFake, NeedsWritingEnabledAgainForEachProgram)
{
    Program(AN_ADDRESS, {0x00});
    LetTimePass(SpiNorFlashPartFake::settings.programMilliseconds);
    (void) Command(WithAddress(PAGE_PROGRAM, AN_ADDRESS + 1, {0x00}));
    BYTES_EQUAL(0xFF, SpiNorFlashPartFake::Stored(AN_ADDRESS + 1, 1)[0]);
}

TEST(SpiNorFlashPartFake, WrapsAProgramThatRunsPastTheEndOfItsPage)
{
    Program(LAST_BYTE_OF_A_PAGE, {0x01, 0x02});
    BYTES_EQUAL(0x01, SpiNorFlashPartFake::Stored(LAST_BYTE_OF_A_PAGE, 1)[0]);
    BYTES_EQUAL(0x02, SpiNorFlashPartFake::Stored(START_OF_THAT_PAGE, 1)[0]);
    BYTES_EQUAL(0xFF, SpiNorFlashPartFake::Stored(LAST_BYTE_OF_A_PAGE + 1, 1)[0]);
}

TEST(SpiNorFlashPartFake, ErasesTheSectorAnAddressIsIn)
{
    SpiNorFlashPartFake::Store(START_OF_A_SECTOR, {0x00});
    SpiNorFlashPartFake::Store(START_OF_A_SECTOR + SECTOR_BYTES - 1, {0x00});
    Erase(SECTOR_ERASE, START_OF_A_SECTOR + 5);
    BYTES_EQUAL(0xFF, SpiNorFlashPartFake::Stored(START_OF_A_SECTOR, 1)[0]);
    BYTES_EQUAL(0xFF, SpiNorFlashPartFake::Stored(START_OF_A_SECTOR + SECTOR_BYTES - 1, 1)[0]);
}

TEST(SpiNorFlashPartFake, ErasesNothingOutsideTheSector)
{
    SpiNorFlashPartFake::Store(START_OF_A_SECTOR - 1, {0x00});
    SpiNorFlashPartFake::Store(START_OF_A_SECTOR + SECTOR_BYTES, {0x00});
    Erase(SECTOR_ERASE, START_OF_A_SECTOR);
    BYTES_EQUAL(0x00, SpiNorFlashPartFake::Stored(START_OF_A_SECTOR - 1, 1)[0]);
    BYTES_EQUAL(0x00, SpiNorFlashPartFake::Stored(START_OF_A_SECTOR + SECTOR_BYTES, 1)[0]);
}

TEST(SpiNorFlashPartFake, RecordsTheErasesItCarriesOut)
{
    Erase(SECTOR_ERASE, START_OF_A_SECTOR);
    LONGS_EQUAL(1, SpiNorFlashPartFake::Erases().size());
    LONGS_EQUAL(START_OF_A_SECTOR, SpiNorFlashPartFake::Erases()[0]);
}

TEST(SpiNorFlashPartFake, RecordsAnEraseAtTheAddressItWasSent)
{
    SpiNorFlashPartFake::settings.capacityBytes = EIGHT_MEBIBYTES;
    Erase(SECTOR_ERASE, EIGHT_MEBIBYTES + START_OF_A_SECTOR);
    LONGS_EQUAL(EIGHT_MEBIBYTES + START_OF_A_SECTOR, SpiNorFlashPartFake::Erases()[0]);
}

TEST(SpiNorFlashPartFake, ErasesAnAddressPastItsEndAsOneWrappedToItsStart)
{
    SpiNorFlashPartFake::settings.capacityBytes = EIGHT_MEBIBYTES;
    SpiNorFlashPartFake::Store(START_OF_A_SECTOR, {0x00});
    Erase(SECTOR_ERASE, EIGHT_MEBIBYTES + START_OF_A_SECTOR);
    BYTES_EQUAL(0xFF, SpiNorFlashPartFake::Stored(START_OF_A_SECTOR, 1)[0]);
}

TEST(SpiNorFlashPartFake, ErasesNothingUnlessWritingWasEnabled)
{
    SpiNorFlashPartFake::Store(START_OF_A_SECTOR, {0x00});
    (void) Command(WithAddress(SECTOR_ERASE, START_OF_A_SECTOR));
    BYTES_EQUAL(0x00, SpiNorFlashPartFake::Stored(START_OF_A_SECTOR, 1)[0]);
}

TEST(SpiNorFlashPartFake, ErasesByTheCommandAndSizeItIsGiven)
{
    SpiNorFlashPartFake::settings.eraseCommand = BLOCK_ERASE;
    SpiNorFlashPartFake::settings.eraseBytes = BLOCK_BYTES;
    SpiNorFlashPartFake::Store(START_OF_A_BLOCK + BLOCK_BYTES - 1, {0x00});
    Erase(BLOCK_ERASE, START_OF_A_BLOCK);
    BYTES_EQUAL(0xFF, SpiNorFlashPartFake::Stored(START_OF_A_BLOCK + BLOCK_BYTES - 1, 1)[0]);
}

TEST(SpiNorFlashPartFake, IgnoresAnEraseCommandThatIsNotItsOwn)
{
    SpiNorFlashPartFake::settings.eraseCommand = BLOCK_ERASE;
    SpiNorFlashPartFake::Store(START_OF_A_SECTOR, {0x00});
    Erase(SECTOR_ERASE, START_OF_A_SECTOR);
    BYTES_EQUAL(0x00, SpiNorFlashPartFake::Stored(START_OF_A_SECTOR, 1)[0]);
    LONGS_EQUAL(0, SpiNorFlashPartFake::Erases().size());
}

TEST(SpiNorFlashPartFake, IsBusyAfterAnErase)
{
    Erase(SECTOR_ERASE, START_OF_A_SECTOR);
    CHECK_TRUE(SpiNorFlashPartFake::IsBusy());
    BYTES_EQUAL(STATUS_BUSY, Status() & STATUS_BUSY);
}

TEST(SpiNorFlashPartFake, IsBusyUntilTheTimeAnEraseTakesHasPassed)
{
    SpiNorFlashPartFake::settings.eraseMilliseconds = ERASE_MILLISECONDS;
    Erase(SECTOR_ERASE, START_OF_A_SECTOR);
    LetTimePass(ERASE_MILLISECONDS - 1);
    CHECK_TRUE(SpiNorFlashPartFake::IsBusy());
    LetTimePass(1);
    CHECK_FALSE(SpiNorFlashPartFake::IsBusy());
}

TEST(SpiNorFlashPartFake, IsBusyUntilTheTimeAProgramTakesHasPassed)
{
    SpiNorFlashPartFake::settings.programMilliseconds = 3;
    Program(AN_ADDRESS, {0x00});
    LetTimePass(2);
    CHECK_TRUE(SpiNorFlashPartFake::IsBusy());
    LetTimePass(1);
    CHECK_FALSE(SpiNorFlashPartFake::IsBusy());
}

TEST(SpiNorFlashPartFake, StartsItsClockWhereATestPutsIt)
{
    SpiNorFlashPartFake::StartClockAt(JUST_BEFORE_THE_WRAP);
    LONGS_EQUAL(JUST_BEFORE_THE_WRAP, SpiNorFlashPartFake::Milliseconds());
    LONGS_EQUAL(JUST_BEFORE_THE_WRAP, bus->Milliseconds(bus));
}

TEST(SpiNorFlashPartFake, WrapsItsCountOfMillisecondsToZero)
{
    SpiNorFlashPartFake::StartClockAt(JUST_BEFORE_THE_WRAP);
    LetTimePass(2);
    LONGS_EQUAL(0, SpiNorFlashPartFake::Milliseconds());
}

TEST(SpiNorFlashPartFake, IsBusyForTheTimeAProgramTakesAcrossAWrapOfTheCount)
{
    SpiNorFlashPartFake::StartClockAt(JUST_BEFORE_THE_WRAP);
    SpiNorFlashPartFake::settings.programMilliseconds = 5;
    Program(AN_ADDRESS, {0x00});
    CHECK_TRUE(SpiNorFlashPartFake::IsBusy());
    LetTimePass(4);
    CHECK_TRUE(SpiNorFlashPartFake::IsBusy());
    LetTimePass(1);
    CHECK_FALSE(SpiNorFlashPartFake::IsBusy());
}

TEST(SpiNorFlashPartFake, IsBusyForTheTimeAnEraseTakesAcrossAWrapOfTheCount)
{
    SpiNorFlashPartFake::StartClockAt(JUST_BEFORE_THE_WRAP);
    SpiNorFlashPartFake::settings.eraseMilliseconds = ERASE_MILLISECONDS;
    Erase(SECTOR_ERASE, START_OF_A_SECTOR);
    CHECK_TRUE(SpiNorFlashPartFake::IsBusy());
    LetTimePass(ERASE_MILLISECONDS - 1);
    CHECK_TRUE(SpiNorFlashPartFake::IsBusy());
    LetTimePass(1);
    CHECK_FALSE(SpiNorFlashPartFake::IsBusy());
}

TEST(SpiNorFlashPartFake, IsBusyFromPowerUpAcrossAWrapOfTheCount)
{
    SpiNorFlashPartFake::StartClockAt(JUST_BEFORE_THE_WRAP);
    SpiNorFlashPartFake::settings.busyAtPowerUpMilliseconds = 5;
    CHECK_TRUE(SpiNorFlashPartFake::IsBusy());
    LetTimePass(4);
    CHECK_TRUE(SpiNorFlashPartFake::IsBusy());
    LetTimePass(1);
    CHECK_FALSE(SpiNorFlashPartFake::IsBusy());
}

TEST(SpiNorFlashPartFake, MovesTimeOnByTheStepItIsGivenAtEachReading)
{
    SpiNorFlashPartFake::settings.millisecondsPerReading = 250;
    LONGS_EQUAL(0, bus->Milliseconds(bus));
    LONGS_EQUAL(250, bus->Milliseconds(bus));
    LONGS_EQUAL(500, SpiNorFlashPartFake::Milliseconds());
}

TEST(SpiNorFlashPartFake, DoesNotMoveTimeOnWhenAskedForItsStatus)
{
    Erase(SECTOR_ERASE, START_OF_A_SECTOR);
    (void) Status();
    (void) Status();
    LONGS_EQUAL(0, SpiNorFlashPartFake::Milliseconds());
}

TEST(SpiNorFlashPartFake, IgnoresACommandThatArrivesWhileItIsBusy)
{
    SpiNorFlashPartFake::Store(AN_ADDRESS, {0x11});
    Erase(SECTOR_ERASE, START_OF_A_SECTOR);
    CHECK_BYTES_EQUAL(Bytes({0xFF}), Command(WithAddress(READ_DATA, AN_ADDRESS), 1));
    LONGS_EQUAL(1, SpiNorFlashPartFake::CommandsWhileBusy());
    LONGS_EQUAL(0, SpiNorFlashPartFake::CountOf(READ_DATA));
}

TEST(SpiNorFlashPartFake, ShowsWritingEnabledUntilAnEraseHasFinished)
{
    SpiNorFlashPartFake::settings.eraseMilliseconds = ERASE_MILLISECONDS;
    Erase(SECTOR_ERASE, START_OF_A_SECTOR);
    BYTES_EQUAL(STATUS_BUSY | STATUS_WRITE_ENABLED, Status());
    LetTimePass(ERASE_MILLISECONDS);
    BYTES_EQUAL(0x00, Status());
}

TEST(SpiNorFlashPartFake, ShowsWritingEnabledUntilAProgramHasFinished)
{
    SpiNorFlashPartFake::settings.programMilliseconds = 3;
    Program(AN_ADDRESS, {0x00});
    BYTES_EQUAL(STATUS_BUSY | STATUS_WRITE_ENABLED, Status());
    LetTimePass(3);
    BYTES_EQUAL(0x00, Status());
}

TEST(SpiNorFlashPartFake, StopsAnsweringAfterTheCommandsItIsGiven)
{
    SpiNorFlashPartFake::GoQuietAfter(2);
    BYTES_EQUAL(0x00, Status());
    CHECK_BYTES_EQUAL(Bytes({0xEF}), Command({READ_JEDEC_ID}, 1));
    BYTES_EQUAL(0xFF, Status());
}

TEST(SpiNorFlashPartFake, CarriesOutNothingOnceItHasStoppedAnswering)
{
    SpiNorFlashPartFake::GoQuietAfter(1);
    (void) Command({WRITE_ENABLE});
    (void) Command(WithAddress(PAGE_PROGRAM, AN_ADDRESS, {0x00}));
    BYTES_EQUAL(0xFF, SpiNorFlashPartFake::Stored(AN_ADDRESS, 1)[0]);
}

TEST(SpiNorFlashPartFake, ForgetsACommandItWasTakingWhenItLostPower)
{
    const uint8_t command = WRITE_ENABLE;
    bus->Select(bus);
    (void) bus->Write(bus, &command, 1);
    SpiNorFlashPartFake::settings.present = false;
    bus->Deselect(bus);
    SpiNorFlashPartFake::settings.present = true;
    BYTES_EQUAL(0x00, Status());
}

TEST(SpiNorFlashPartFake, IsBusyFromPowerUpForTheTimeItIsGiven)
{
    SpiNorFlashPartFake::settings.busyAtPowerUpMilliseconds = 5;
    CHECK_TRUE(SpiNorFlashPartFake::IsBusy());
    LetTimePass(5);
    CHECK_FALSE(SpiNorFlashPartFake::IsBusy());
}

TEST(SpiNorFlashPartFake, StaysBusyHoweverLongWhenToldTo)
{
    SpiNorFlashPartFake::settings.staysBusy = true;
    SpiNorFlashPartFake::settings.millisecondsPerReading = 60000;
    LetTimePass(10);
    CHECK_TRUE(SpiNorFlashPartFake::IsBusy());
}

TEST(SpiNorFlashPartFake, FailsTheNextWriteAndNoOther)
{
    const uint8_t command = WRITE_ENABLE;
    SpiNorFlashPartFake::FailNextWrite();
    bus->Select(bus);
    CHECK_FALSE(bus->Write(bus, &command, 1));
    CHECK_TRUE(bus->Write(bus, &command, 1));
    bus->Deselect(bus);
}

TEST(SpiNorFlashPartFake, ShowsThePartNoneOfAWriteThatFails)
{
    SpiNorFlashPartFake::FailNextWrite();
    (void) Command({WRITE_ENABLE});
    BYTES_EQUAL(0x00, Status());
}

TEST(SpiNorFlashPartFake, FailsTheNextReadAndNoOther)
{
    uint8_t in = 0;
    SpiNorFlashPartFake::FailNextRead();
    CHECK_FALSE(bus->Read(bus, &in, 1));
    CHECK_TRUE(bus->Read(bus, &in, 1));
}

TEST(SpiNorFlashPartFake, FailsTheWriteThatManyCallsFromNow)
{
    const uint8_t command = WRITE_ENABLE;
    SpiNorFlashPartFake::FailWriteAt(2);
    CHECK_TRUE(bus->Write(bus, &command, 1));
    CHECK_FALSE(bus->Write(bus, &command, 1));
    CHECK_TRUE(bus->Write(bus, &command, 1));
}

TEST(SpiNorFlashPartFake, FailsTheReadThatManyCallsFromNow)
{
    uint8_t in = 0;
    SpiNorFlashPartFake::FailReadAt(2);
    CHECK_TRUE(bus->Read(bus, &in, 1));
    CHECK_FALSE(bus->Read(bus, &in, 1));
    CHECK_TRUE(bus->Read(bus, &in, 1));
}

TEST(SpiNorFlashPartFake, StartsEachTestAsABlankPartWithNoHistory)
{
    Program(AN_ADDRESS, {0x00});
    SpiNorFlashPartFake::Reset();
    BYTES_EQUAL(0xFF, SpiNorFlashPartFake::Stored(AN_ADDRESS, 1)[0]);
    LONGS_EQUAL(0, SpiNorFlashPartFake::Commands().size());
    LONGS_EQUAL(0, SpiNorFlashPartFake::Milliseconds());
}
