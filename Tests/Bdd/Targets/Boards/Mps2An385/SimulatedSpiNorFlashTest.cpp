#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

#include "ErrorHandlerFake.h"
#include "SimulatedSpiNorFlash.h"
#include "SolidSyslogNorFlash.h"
#include "SolidSyslogNullNorFlash.h"
#include "SolidSyslogSpiNorFlash.h"
#include "SolidSyslogSpiNorFlashBus.h"
#include "SpiNorFlashPartFake.h"
#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

using Bytes = std::vector<uint8_t>;

namespace
{
enum
{
    PART_BYTES = 16 * 1024 * 1024
};

/* What the part keeps its bytes in: here memory, and every byte of it zero
 * until it is written. */
std::map<uint32_t, uint8_t> held;
int mediumReads;
int mediumWrites;
bool mediumFails;

/* False for a span that is not all within the part, as well as when a test
 * has said the medium fails. */
bool MediumHas(uint32_t offset, size_t count)
{
    return !mediumFails && (count <= PART_BYTES) && (offset <= (PART_BYTES - count));
}

bool MediumRead(struct SimulatedSpiNorFlashMedium* base, uint32_t offset, void* buffer, size_t count)
{
    (void) base;
    mediumReads++;
    auto* bytes = static_cast<uint8_t*>(buffer);
    for (size_t i = 0; MediumHas(offset, count) && (i < count); i++)
    {
        auto found = held.find(offset + static_cast<uint32_t>(i));
        bytes[i] = (found == held.end()) ? 0x00 : found->second;
    }
    return MediumHas(offset, count);
}

bool MediumWrite(struct SimulatedSpiNorFlashMedium* base, uint32_t offset, const void* buffer, size_t count)
{
    (void) base;
    mediumWrites++;
    const auto* bytes = static_cast<const uint8_t*>(buffer);
    for (size_t i = 0; MediumHas(offset, count) && (i < count); i++)
    {
        held[offset + static_cast<uint32_t>(i)] = bytes[i];
    }
    return MediumHas(offset, count);
}

struct SimulatedSpiNorFlashMedium medium = {MediumRead, MediumWrite};

/* The count of milliseconds, and how far it moves on each time it is read:
 * not at all until a test has something that waits. */
uint32_t now;
uint32_t millisecondsPerReading;

uint32_t Milliseconds()
{
    now += millisecondsPerReading;
    return now;
}

int whileBusyCalls;

void WhileBusy(struct SolidSyslogSpiNorFlashBus* base)
{
    (void) base;
    whileBusyCalls++;
}
} // namespace

enum
{
    PAGE_PROGRAM = 0x02,
    READ_DATA = 0x03,
    READ_STATUS = 0x05,
    WRITE_ENABLE = 0x06,
    SECTOR_ERASE = 0x20,
    READ_SFDP = 0x5A,

    ERASED = 0xFF
};

/* A macro, so that a failure reports the test's own line. */
#define CHECK_BYTES(expected, actual)                                                 \
    {                                                                                 \
        const Bytes& expectedBytes = expected;                                        \
        const Bytes& actualBytes = actual;                                            \
        LONGS_EQUAL(expectedBytes.size(), actualBytes.size());                        \
        MEMCMP_EQUAL(expectedBytes.data(), actualBytes.data(), expectedBytes.size()); \
    }

// clang-format off
TEST_BASE(SimulatedPartTestBase)
{
    struct SolidSyslogSpiNorFlashBus* bus = nullptr;

    void PowerUpANewPart()
    {
        mediumReads = 0;
        mediumWrites = 0;
        mediumFails = false;
        now = 0;
        millisecondsPerReading = 0;
        whileBusyCalls = 0;
        PowerUp();
    }

    void PowerUp()
    {
        const struct SimulatedSpiNorFlashConfig config = {&medium, Milliseconds};
        bus = SimulatedSpiNorFlash_PowerUp(&config);
    }

    /* Memory the leak check counts. */
    static void ForgetThePart()
    {
        held.clear();
    }
};

TEST_GROUP_BASE(SimulatedSpiNorFlash, SimulatedPartTestBase)
{
    void setup() override
    {
        PowerUpANewPart();
    }

    void teardown() override
    {
        ForgetThePart();
    }

    /* Puts bytes straight into the medium, as they are. */
    static void Hold(uint32_t offset, const Bytes& bytes)
    {
        for (size_t i = 0; i < bytes.size(); i++)
        {
            held[offset + static_cast<uint32_t>(i)] = bytes[i];
        }
    }

    /* One command and what the part answers it with, within one select. */
    [[nodiscard]] Bytes Ask(const Bytes& request, size_t answerCount) const
    {
        Bytes answer(answerCount, 0x00);
        bus->Select(bus);
        bus->Write(bus, request.data(), request.size());
        bus->Read(bus, answer.data(), answer.size());
        bus->Deselect(bus);
        return answer;
    }

    /* One command that is not answered. */
    void Send(const Bytes& request) const
    {
        bus->Select(bus);
        bus->Write(bus, request.data(), request.size());
        bus->Deselect(bus);
    }

    /* A program, and the time it takes. */
    void Program(uint32_t address, const Bytes& data) const
    {
        BeginProgram(address, data);
        now += SIMULATED_SPI_NOR_FLASH_PROGRAM_MILLISECONDS;
    }

    /* Sends the two commands a program takes, and leaves the part busy. */
    void BeginProgram(uint32_t address, const Bytes& data) const
    {
        Send({WRITE_ENABLE});
        SendProgram(address, data);
    }

    /* An erase, and the time it takes. */
    void Erase(uint32_t address) const
    {
        BeginErase(address);
        now += SIMULATED_SPI_NOR_FLASH_ERASE_MILLISECONDS;
    }

    void BeginErase(uint32_t address) const
    {
        Send({WRITE_ENABLE});
        Send(WithAddress(SECTOR_ERASE, address));
    }

    void SendProgram(uint32_t address, const Bytes& data) const
    {
        Bytes request = WithAddress(PAGE_PROGRAM, address);
        request.insert(request.end(), data.begin(), data.end());
        Send(request);
    }

    [[nodiscard]] Bytes ReadData(uint32_t address, size_t count) const
    {
        return Ask(WithAddress(READ_DATA, address), count);
    }

    /* The part ignores the byte after the address. */
    [[nodiscard]] Bytes ReadSfdp(uint32_t address, size_t count) const
    {
        Bytes request = WithAddress(READ_SFDP, address);
        request.push_back(0x00);
        return Ask(request, count);
    }

    [[nodiscard]] uint8_t Status() const
    {
        return Ask({READ_STATUS}, 1).front();
    }

    static Bytes WithAddress(uint8_t command, uint32_t address)
    {
        return {command, static_cast<uint8_t>(address >> 16U), static_cast<uint8_t>(address >> 8U),
                static_cast<uint8_t>(address)};
    }
};

// clang-format on

TEST(SimulatedSpiNorFlash, ANewPartReadsAsErased)
{
    CHECK_BYTES(Bytes({ERASED, ERASED}), ReadData(0, 2));
}

TEST(SimulatedSpiNorFlash, ANewPartIsReadyAndHasWritingDisabled)
{
    BYTES_EQUAL(0x00, Status());
}

TEST(SimulatedSpiNorFlash, ItsSfdpBeginsWithTheSignature)
{
    CHECK_BYTES(Bytes({'S', 'F', 'D', 'P'}), ReadSfdp(0x000000, 4));
}

/* The table a W25Q128JV answers with, as it was read from one, and the one
 * the SpiNorFlash pack's own tests give their fake part. Every byte outside
 * its header and its basic table reads as erased. */
TEST(SimulatedSpiNorFlash, ItsSfdpIsTheW25Q128JvsThePacksTestsUse)
{
    const SpiNorFlashPartFake::SfdpTable table = SpiNorFlashPartFake::W25Q128JvSfdp();
    CHECK_BYTES(Bytes(table.begin(), table.end()), ReadSfdp(0x000000, table.size()));
}

TEST(SimulatedSpiNorFlash, ItsSfdpRepeatsEvery256Bytes)
{
    CHECK_BYTES(Bytes({ERASED, ERASED, 'S', 'F'}), ReadSfdp(0x0000FE, 4));
    CHECK_BYTES(Bytes({'S', 'F', 'D', 'P'}), ReadSfdp(0x012300, 4));
}

TEST(SimulatedSpiNorFlash, ItsSfdpIsNotAnsweredUntilTheByteAfterTheAddressHasBeenSent)
{
    CHECK_BYTES(Bytes({0xFF}), Ask(WithAddress(READ_SFDP, 0x000000), 1));
}

/* So a medium that has never been written, which holds zeros, is an erased
 * part. */
TEST(SimulatedSpiNorFlash, ItReadsTheComplementOfWhatItsMediumHolds)
{
    Hold(0x012345, {0xFE, 0xED, 0x00});
    CHECK_BYTES(Bytes({0x01, 0x12, 0xFF}), ReadData(0x012345, 3));
}

/* Each call of the medium on the QEMU machine is a trap to the host. */
TEST(SimulatedSpiNorFlash, AReadTakesItsBytesFromTheMediumInOneCall)
{
    (void) ReadData(0x000000, 300);
    LONGS_EQUAL(1, mediumReads);
}

TEST(SimulatedSpiNorFlash, AReadRunsOnFromWhereTheLastEnded)
{
    Hold(0x000010, {0xFE, 0xFD, 0xFC, 0xFB});
    const Bytes request = WithAddress(READ_DATA, 0x000010);
    Bytes first(2, 0x00);
    Bytes second(2, 0x00);

    bus->Select(bus);
    bus->Write(bus, request.data(), request.size());
    bus->Read(bus, first.data(), first.size());
    bus->Read(bus, second.data(), second.size());
    bus->Deselect(bus);

    CHECK_BYTES(Bytes({0x01, 0x02}), first);
    CHECK_BYTES(Bytes({0x03, 0x04}), second);
}

TEST(SimulatedSpiNorFlash, AReadPastTheEndOfThePartGoesRoundToItsStart)
{
    Hold(PART_BYTES - 1, {0xFE});
    Hold(0x000000, {0xFD, 0xFC});
    CHECK_BYTES(Bytes({0x01, 0x02, 0x03}), ReadData(PART_BYTES - 1, 3));
}

TEST(SimulatedSpiNorFlash, AReadFailsWhenTheMediumDoes)
{
    const Bytes request = WithAddress(READ_DATA, 0x000000);
    uint8_t answer = 0x00;
    mediumFails = true;

    bus->Select(bus);
    bus->Write(bus, request.data(), request.size());
    CHECK_FALSE(bus->Read(bus, &answer, sizeof(answer)));
    bus->Deselect(bus);
}

TEST(SimulatedSpiNorFlash, AReadWithoutItsWholeAddressIsNotAnswered)
{
    Hold(0x000000, {0xFE});
    CHECK_BYTES(Bytes({0xFF}), Ask({READ_DATA, 0x00, 0x00}, 1));
    LONGS_EQUAL(0, mediumReads);
}

TEST(SimulatedSpiNorFlash, ItAnswersNothingWhileItIsNotSelected)
{
    const uint8_t request = READ_STATUS;
    uint8_t answer = 0x00;

    bus->Write(bus, &request, sizeof(request));
    bus->Read(bus, &answer, sizeof(answer));

    BYTES_EQUAL(0xFF, answer);
}

TEST(SimulatedSpiNorFlash, ItStopsAnsweringACommandWhenItIsDeselected)
{
    uint8_t answer = 0x00;
    (void) Status();

    bus->Read(bus, &answer, sizeof(answer));

    BYTES_EQUAL(0xFF, answer);
}

TEST(SimulatedSpiNorFlash, EachSelectBeginsANewCommand)
{
    Hold(0x000000, {0xFE});
    (void) Status();
    CHECK_BYTES(Bytes({0x01}), ReadData(0x000000, 1));
}

TEST(SimulatedSpiNorFlash, TheBusTellsTheTimeItWasGiven)
{
    now = 1234;
    LONGS_EQUAL(1234, bus->Milliseconds(bus));
}

TEST(SimulatedSpiNorFlash, EnablingWritingShowsInItsStatus)
{
    Send({WRITE_ENABLE});
    BYTES_EQUAL(0x02, Status());
}

TEST(SimulatedSpiNorFlash, AProgramPutsItsBytesInThePart)
{
    Program(0x012340, {0x12, 0x34, 0x56});
    CHECK_BYTES(Bytes({0x12, 0x34, 0x56, ERASED}), ReadData(0x012340, 4));
}

TEST(SimulatedSpiNorFlash, AProgramClearsBitsAndSetsNone)
{
    Program(0x000100, {0xF0});
    Program(0x000100, {0x3C});
    CHECK_BYTES(Bytes({0x30}), ReadData(0x000100, 1));
}

TEST(SimulatedSpiNorFlash, AProgramIsIgnoredUnlessWritingWasEnabled)
{
    SendProgram(0x000100, {0x12});
    CHECK_BYTES(Bytes({ERASED}), ReadData(0x000100, 1));
}

TEST(SimulatedSpiNorFlash, AProgramPastTheEndOfItsPageGoesRoundToThePagesStart)
{
    Program(0x0001FE, {0x01, 0x02, 0x03, 0x04});
    CHECK_BYTES(Bytes({0x03, 0x04, ERASED}), ReadData(0x000100, 3));
    CHECK_BYTES(Bytes({0x01, 0x02, ERASED}), ReadData(0x0001FE, 3));
}

/* As the part does: it keeps a page of what it is sent, and what comes after
 * that goes over the first of it. */
TEST(SimulatedSpiNorFlash, AProgramOfMoreThanAPageKeepsTheLastPageOfIt)
{
    Bytes data(258, 0x55);
    data[256] = 0x0A;
    data[257] = 0x0B;
    Program(0x000200, data);
    CHECK_BYTES(Bytes({0x0A, 0x0B, 0x55}), ReadData(0x000200, 3));
    CHECK_BYTES(Bytes({0x55, ERASED}), ReadData(0x0002FF, 2));
}

TEST(SimulatedSpiNorFlash, ItIsBusyWithWritingEnabledForATimeAfterAProgramBegins)
{
    BeginProgram(0x000100, {0x12});
    BYTES_EQUAL(0x03, Status());

    now += SIMULATED_SPI_NOR_FLASH_PROGRAM_MILLISECONDS;
    BYTES_EQUAL(0x00, Status());
}

TEST(SimulatedSpiNorFlash, BeingBusyHoldsAcrossAWrapOfTheCount)
{
    now = UINT32_MAX;
    BeginProgram(0x000100, {0x12});
    BYTES_EQUAL(0x03, Status());

    now += SIMULATED_SPI_NOR_FLASH_PROGRAM_MILLISECONDS;
    BYTES_EQUAL(0x00, Status());
}

TEST(SimulatedSpiNorFlash, WhileItIsBusyItAnswersOnlyForItsStatus)
{
    Hold(0x000000, {0xFE});
    BeginProgram(0x000100, {0x12});

    CHECK_BYTES(Bytes({0xFF}), ReadData(0x000000, 1));
    CHECK_BYTES(Bytes({0xFF}), ReadSfdp(0x000000, 1));
}

TEST(SimulatedSpiNorFlash, WhileItIsBusyItIgnoresAProgram)
{
    BeginProgram(0x000100, {0x12});
    BeginProgram(0x000200, {0x34});
    now += SIMULATED_SPI_NOR_FLASH_PROGRAM_MILLISECONDS;

    CHECK_BYTES(Bytes({ERASED}), ReadData(0x000200, 1));
}

TEST(SimulatedSpiNorFlash, WhileItIsBusyItIgnoresAnErase)
{
    Hold(0x001000, {0xFE});
    BeginProgram(0x000100, {0x12});
    BeginErase(0x001000);
    now += SIMULATED_SPI_NOR_FLASH_PROGRAM_MILLISECONDS;

    CHECK_BYTES(Bytes({0x01}), ReadData(0x001000, 1));
}

TEST(SimulatedSpiNorFlash, AnEraseSetsTheSectorTheAddressIsInToOnes)
{
    Hold(0x000FFF, {0xFE, 0xFD});
    Hold(0x001FFF, {0xFC, 0xFB});

    Erase(0x001234);

    CHECK_BYTES(Bytes({0x01, ERASED}), ReadData(0x000FFF, 2));
    CHECK_BYTES(Bytes({ERASED, 0x04}), ReadData(0x001FFF, 2));
}

TEST(SimulatedSpiNorFlash, AnEraseIsIgnoredUnlessWritingWasEnabled)
{
    Hold(0x001000, {0xFE});
    Send(WithAddress(SECTOR_ERASE, 0x001000));
    CHECK_BYTES(Bytes({0x01}), ReadData(0x001000, 1));
}

TEST(SimulatedSpiNorFlash, AnEraseWithoutItsWholeAddressIsIgnored)
{
    Hold(0x000000, {0xFE});
    Send({WRITE_ENABLE});
    Send({SECTOR_ERASE, 0x00, 0x00});
    CHECK_BYTES(Bytes({0x01}), ReadData(0x000000, 1));
}

TEST(SimulatedSpiNorFlash, ItIsBusyWithWritingEnabledForATimeAfterAnEraseBegins)
{
    BeginErase(0x001000);
    BYTES_EQUAL(0x03, Status());

    now += SIMULATED_SPI_NOR_FLASH_ERASE_MILLISECONDS - 1;
    BYTES_EQUAL(0x03, Status());

    now += 1;
    BYTES_EQUAL(0x00, Status());
}

/* Each call of the medium on the QEMU machine is a trap to the host. */
TEST(SimulatedSpiNorFlash, AnEraseIsOneCallOfTheMedium)
{
    Erase(0x001000);
    LONGS_EQUAL(1, mediumWrites);
    LONGS_EQUAL(0, mediumReads);
}

TEST(SimulatedSpiNorFlash, PoweringItUpLeavesTheBusWithNoWhileBusy)
{
    bus->WhileBusy = WhileBusy;
    PowerUp();
    CHECK_TRUE(bus->WhileBusy == nullptr);
}

TEST(SimulatedSpiNorFlash, PoweredUpAgainItHoldsWhatItHeldAndIsReadyWithWritingDisabled)
{
    Program(0x000100, {0x12});
    BeginErase(0x001000);

    PowerUp();

    BYTES_EQUAL(0x00, Status());
    CHECK_BYTES(Bytes({0x12}), ReadData(0x000100, 1));
}

/* The library's own driver over the part, as it is on the QEMU machine. The
 * count of milliseconds moves on at each reading, so what the driver waits
 * for comes. */
// clang-format off
TEST_GROUP_BASE(SimulatedSpiNorFlashUnderThePack, SimulatedPartTestBase)
{
    struct SolidSyslogNorFlash* flash = nullptr;

    void setup() override
    {
        PowerUpANewPart();
        millisecondsPerReading = 1;
        ErrorHandlerFake_Install(nullptr);
        const struct SolidSyslogSpiNorFlashConfig config = {bus};
        flash = SolidSyslogSpiNorFlash_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogSpiNorFlash_Destroy(flash);
        ForgetThePart();
    }
};

// clang-format on

/* It learns the part from its SFDP. */
TEST(SimulatedSpiNorFlashUnderThePack, ThePackTakesItForAW25Q128Jv)
{
    CHECK(flash != SolidSyslogNullNorFlash_Get());
    LONGS_EQUAL(4096, SolidSyslogNorFlash_GetBlockSize(flash));
    LONGS_EQUAL(4096, SolidSyslogNorFlash_GetBlockCount(flash));
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SimulatedSpiNorFlashUnderThePack, WhatThePackProgramsItReadsBack)
{
    Bytes written(600);
    for (size_t i = 0; i < written.size(); i++)
    {
        written[i] = static_cast<uint8_t>(i);
    }
    Bytes read(written.size(), 0x00);

    CHECK_TRUE(SolidSyslogNorFlash_Program(flash, 3, 100, written.data(), written.size()));
    CHECK_TRUE(SolidSyslogNorFlash_Read(flash, 3, 100, read.data(), read.size()));

    CHECK_BYTES(written, read);
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SimulatedSpiNorFlashUnderThePack, WhatThePackErasesReadsAsErased)
{
    const Bytes written(600, 0x00);
    Bytes read(written.size(), 0x00);
    SolidSyslogNorFlash_Program(flash, 3, 100, written.data(), written.size());

    CHECK_TRUE(SolidSyslogNorFlash_Erase(flash, 3));
    CHECK_TRUE(SolidSyslogNorFlash_Read(flash, 3, 100, read.data(), read.size()));

    CHECK_BYTES(Bytes(written.size(), ERASED), read);
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

/* So the pack's wait, and the bus's WhileBusy in it, run wherever this part
 * is the one beneath it. */
TEST(SimulatedSpiNorFlashUnderThePack, ThePackFindsItBusyAndWaits)
{
    bus->WhileBusy = WhileBusy;
    SolidSyslogNorFlash_Erase(flash, 3);
    CHECK_TRUE(whileBusyCalls > 0);
}
