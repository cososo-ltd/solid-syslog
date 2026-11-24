#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

#include "SimulatedSpiNorFlash.h"
#include "SolidSyslogNorFlash.h"
#include "SolidSyslogNullNorFlash.h"
#include "SolidSyslogSpiNorFlash.h"
#include "SolidSyslogSpiNorFlashBus.h"
#include "CppUTest/TestHarness.h"

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

struct SimulatedSpiNorFlashMedium medium = {MediumRead};

uint32_t now;

uint32_t Milliseconds()
{
    return now;
}
} // namespace

enum
{
    READ_DATA = 0x03,
    READ_STATUS = 0x05,
    READ_SFDP = 0x5A,

    ERASED = 0xFF
};

// NOLINTBEGIN(cppcoreguidelines-macro-usage) -- macros so a failure reports the test's own line
#define CHECK_BYTES(expected, actual)                                                 \
    {                                                                                 \
        const Bytes expectedBytes = expected;                                         \
        const Bytes actualBytes = actual;                                             \
        LONGS_EQUAL(expectedBytes.size(), actualBytes.size());                        \
        MEMCMP_EQUAL(expectedBytes.data(), actualBytes.data(), expectedBytes.size()); \
    }

// NOLINTEND(cppcoreguidelines-macro-usage)

// clang-format off
TEST_GROUP(SimulatedSpiNorFlash)
{
    struct SolidSyslogSpiNorFlashBus* bus = nullptr;

    void setup() override
    {
        mediumReads = 0;
        mediumFails = false;
        now = 0;
        const struct SimulatedSpiNorFlashConfig config = {&medium, Milliseconds};
        bus = SimulatedSpiNorFlash_PowerUp(&config);
    }

    void teardown() override
    {
        /* Memory the leak check counts. */
        held.clear();
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
    Bytes Ask(const Bytes& request, size_t answerCount) const
    {
        Bytes answer(answerCount, 0x00);
        bus->Select(bus);
        bus->Write(bus, request.data(), request.size());
        bus->Read(bus, answer.data(), answer.size());
        bus->Deselect(bus);
        return answer;
    }

    Bytes ReadData(uint32_t address, size_t count) const
    {
        return Ask(WithAddress(READ_DATA, address), count);
    }

    /* The part ignores the byte after the address. */
    Bytes ReadSfdp(uint32_t address, size_t count) const
    {
        Bytes request = WithAddress(READ_SFDP, address);
        request.push_back(0x00);
        return Ask(request, count);
    }

    uint8_t Status() const
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

/* The table a W25Q128JV answers with, as it was read from one: SFDP 1.5, one
 * parameter header, and a basic table of sixteen DWORDs at 0x80. */
TEST(SimulatedSpiNorFlash, ItsSfdpIsAW25Q128Jvs)
{
    CHECK_BYTES(
        Bytes({0x53, 0x46, 0x44, 0x50, 0x05, 0x01, 0x00, 0xFF, 0x00, 0x05, 0x01, 0x10, 0x80, 0x00, 0x00, 0xFF}),
        ReadSfdp(0x000000, 16)
    );
    CHECK_BYTES(
        Bytes({0xE5, 0x20, 0xF9, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x44, 0xEB, 0x08, 0x6B, 0x08, 0x3B, 0x42, 0xBB,
               0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x40, 0xEB, 0x0C, 0x20, 0x0F, 0x52,
               0x10, 0xD8, 0x00, 0x00, 0x36, 0x02, 0xA6, 0x00, 0x82, 0xEA, 0x14, 0xC9, 0xE9, 0x63, 0x76, 0x33,
               0x7A, 0x75, 0x7A, 0x75, 0xF7, 0xA2, 0xD5, 0x5C, 0x19, 0xF7, 0x4D, 0xFF, 0xE9, 0x30, 0xF8, 0x80}),
        ReadSfdp(0x000080, 64)
    );
}

TEST(SimulatedSpiNorFlash, ItsSfdpReadsAsErasedOutsideTheHeaderAndTheTable)
{
    CHECK_BYTES(Bytes(0x80 - 0x10, ERASED), ReadSfdp(0x000010, 0x80 - 0x10));
    CHECK_BYTES(Bytes(0x100 - 0xC0, ERASED), ReadSfdp(0x0000C0, 0x100 - 0xC0));
}

TEST(SimulatedSpiNorFlash, ItsSfdpRepeatsEvery256Bytes)
{
    CHECK_BYTES(Bytes({ERASED, ERASED, 'S', 'F'}), ReadSfdp(0x0000FE, 4));
    CHECK_BYTES(Bytes({'S', 'F', 'D', 'P'}), ReadSfdp(0x012300, 4));
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
    ReadData(0x000000, 300);
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
    Status();

    bus->Read(bus, &answer, sizeof(answer));

    BYTES_EQUAL(0xFF, answer);
}

TEST(SimulatedSpiNorFlash, EachSelectBeginsANewCommand)
{
    Hold(0x000000, {0xFE});
    Status();
    CHECK_BYTES(Bytes({0x01}), ReadData(0x000000, 1));
}

TEST(SimulatedSpiNorFlash, TheBusTellsTheTimeItWasGiven)
{
    now = 1234;
    LONGS_EQUAL(1234, bus->Milliseconds(bus));
}

/* The library's own driver, which learns the part from its SFDP. */
TEST(SimulatedSpiNorFlash, TheSpiNorFlashPackTakesItForAW25Q128Jv)
{
    const struct SolidSyslogSpiNorFlashConfig packConfig = {bus};
    struct SolidSyslogNorFlash* flash = SolidSyslogSpiNorFlash_Create(&packConfig);

    CHECK(flash != SolidSyslogNullNorFlash_Get());
    LONGS_EQUAL(4096, SolidSyslogNorFlash_GetBlockSize(flash));
    LONGS_EQUAL(4096, SolidSyslogNorFlash_GetBlockCount(flash));

    SolidSyslogSpiNorFlash_Destroy(flash);
}
