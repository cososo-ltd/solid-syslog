#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "ErrorHandlerFake.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogNorFlash.h"
#include "SolidSyslogNorFlashBlockDevice.h"
#include "SolidSyslogNorFlashCategories.h"
#include "SolidSyslogNullNorFlash.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSpiNorFlash.h"
#include "SolidSyslogSpiNorFlashBus.h"
#include "SolidSyslogSpiNorFlashErrors.h"
#include "SolidSyslogTunables.h"
#include "SpiNorFlashPartFake.h"
#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

enum
{
    WRITE_STATUS = 0x01,
    WRITE_ENABLE = 0x06,

    SECTOR_BYTES = 4096,
    SECTORS_IN_SIXTEEN_MEBIBYTES = 4096,
    BLOCK_BYTES = 65536,
    BLOCKS_IN_SIXTEEN_MEBIBYTES = 256,

    READ_SFDP = 0x5A,

    /* Where the fixture's SFDP keeps things. */
    SFDP_MAJOR_REVISION = 5,
    SFDP_FURTHER_PARAMETER_HEADERS = 6,
    SFDP_HEADER_BYTES = 8,
    PARAMETER_HEADER_BYTES = 8,
    PARAMETER_HEADER_ID_HIGH = 7,
    /* The SFDP header and the basic table's parameter header are read as
     * one, and then the nine DWORDs of the table that the flash uses. */
    FIRST_SFDP_READ_BYTES = SFDP_HEADER_BYTES + PARAMETER_HEADER_BYTES,
    TABLE_BYTES_READ = 36,

    /* The parameter IDs JESD216 gives a sector map, in two bytes, and the
     * low byte of two of its other tables. */
    SECTOR_MAP_ID_LOW = 0x81,
    JEDEC_TABLE_ID_HIGH = 0xFF,
    FOUR_BYTE_ADDRESS_TABLE_ID_LOW = 0x84,
    STATUS_AND_CONTROL_TABLE_ID_LOW = 0x87,
    A_MAKERS_TABLE_ID_HIGH = 0x01,

    TABLE_ID_LOW = 8,
    TABLE_MAJOR_REVISION = 10,
    TABLE_LENGTH_DWORDS = 11,
    TABLE_POINTER = 12,
    TABLE_ID_HIGH = 15,
    TABLE_ADDRESS = 0x80,
    TABLE_BYTES = 64,
    ANOTHER_TABLE_ADDRESS = 0x30,
    DWORDS_IN_THE_FIRST_JESD216 = 9,

    DWORD_ADDRESS_BYTES = 1,
    DWORD_DENSITY = 2,
    DWORD_ERASE_TYPES_1_AND_2 = 8,
    DWORD_ERASE_TYPES_3_AND_4 = 9,

    ERASE_4K_BY_20 = 0x200C,
    ERASE_64K_BY_D8 = 0xD810,
    ERASE_32M_BY_DC = 0xDC19,

    /* What a start asks of a part that is ready: its status, its SFDP header
     * and its table. Each is one command, of one write and one read. */
    COMMANDS_IN_A_START = 3,
    /* Every bit of the first status register but busy and write enabled. */
    STATUS_BITS_THE_PART_DOES_NOT_SET_ITSELF = 0xFC,
    TRANSFERS_IN_A_START = COMMANDS_IN_A_START,
    /* Its status and its SFDP header come before any further parameter
     * header is asked for. */
    TRANSFERS_BEFORE_A_FURTHER_PARAMETER_HEADER = 2,
    START_TIMEOUT_MILLISECONDS = 3000
};

static const uint32_t ADDRESS_BYTES_MASK = 0x00060000U;
static const uint32_t THREE_OR_FOUR_ADDRESS_BYTES = 0x00020000U;
static const uint32_t FOUR_ADDRESS_BYTES_ONLY = 0x00040000U;
static const uint32_t ADDRESS_BYTES_RESERVED = 0x00060000U;
static const uint32_t DENSITY_8_MEBIBYTES = 0x03FFFFFFU;
static const uint32_t DENSITY_32_MEBIBYTES = 0x0FFFFFFFU;
static const uint32_t DENSITY_AS_AN_EXPONENT = 0x80000000U;
static const uint32_t EXPONENT_OF_32_KIBIBYTES = 18U;
static const uint32_t EXPONENT_OF_2_MEBIBYTES = 24U;
static const uint32_t EXPONENT_OF_16_MEBIBYTES = 27U;
static const uint32_t EXPONENT_OF_32_MEBIBYTES = 28U;

/* Macros, so a failure is reported at the test's own line. A handle that is
 * NULL is no more a flash than the Null flash is. */
#define CHECK_IS_A_FLASH(handle)                               \
    {                                                          \
        CHECK_TRUE((handle) != nullptr);                       \
        CHECK_TRUE((handle) != SolidSyslogNullNorFlash_Get()); \
    }

#define CHECK_CREATED() CHECK_IS_A_FLASH(flash)

#define CHECK_NOT_CREATED() POINTERS_EQUAL(SolidSyslogNullNorFlash_Get(), flash)

/* Create answered the Null flash and said why, once, as a config that cannot
 * work. */
#define CHECK_REFUSED(detail)                   \
    {                                           \
        CHECK_NOT_CREATED();                    \
        CHECK_ERROR_REPORTED_ONCE(              \
            SOLIDSYSLOG_SEVERITY_CRITICAL,      \
            &SolidSyslogSpiNorFlashErrorSource, \
            SOLIDSYSLOG_CAT_BAD_CONFIG,         \
            (detail)                            \
        );                                      \
    }

#define CHECK_IO_FAILURE_REPORTED(detail)    \
    CHECK_ERROR_REPORTED_ONCE(               \
        SOLIDSYSLOG_SEVERITY_ERROR,          \
        &SolidSyslogSpiNorFlashErrorSource,  \
        SOLIDSYSLOG_CAT_NOR_FLASH_IO_FAILED, \
        (detail)                             \
    )

/* Create answered the Null flash and said why, once, as a part or a bus that
 * failed. */
#define CHECK_DID_NOT_START(detail)        \
    {                                      \
        CHECK_NOT_CREATED();               \
        CHECK_IO_FAILURE_REPORTED(detail); \
    }

#define CHECK_SFDP_READ(expectedAddress, expectedLength, read) \
    {                                                          \
        LONGS_EQUAL((expectedAddress), (read).address);        \
        LONGS_EQUAL((expectedLength), (read).length);          \
    }

// clang-format off
TEST_BASE(SpiNorFlashTestBase)
{
    struct SolidSyslogNorFlash* flash = nullptr;

    static void SetupPart()
    {
        SpiNorFlashPartFake::Reset();
        ErrorHandlerFake_Install(nullptr);
    }

    void TeardownPart()
    {
        DestroyFlash();
        /* The fake holds what it was sent in memory the leak check counts. */
        SpiNorFlashPartFake::Release();
    }

    void CreateFlash()
    {
        const struct SolidSyslogSpiNorFlashConfig config = {SpiNorFlashPartFake::Bus()};
        flash = SolidSyslogSpiNorFlash_Create(&config);
    }

    void DestroyFlash()
    {
        if ((flash != nullptr) && (flash != SolidSyslogNullNorFlash_Get()))
        {
            SolidSyslogSpiNorFlash_Destroy(flash);
        }
        flash = nullptr;
    }

    /* DWORDs are numbered from 1, as JESD216 numbers them. */
    static void SetDword(size_t number, uint32_t value)
    {
        const size_t at = TABLE_ADDRESS + ((number - 1U) * 4U);
        for (size_t i = 0; i < 4U; i++)
        {
            SpiNorFlashPartFake::settings.sfdp.at(at + i) = static_cast<uint8_t>(value >> (8U * i));
        }
    }

    static uint32_t Dword(size_t number)
    {
        const size_t at = TABLE_ADDRESS + ((number - 1U) * 4U);
        uint32_t value = 0;
        for (size_t i = 0; i < 4U; i++)
        {
            value |= static_cast<uint32_t>(SpiNorFlashPartFake::settings.sfdp.at(at + i)) << (8U * i);
        }
        return value;
    }

    /* Up to four erase types, each a size as a power of two in its low byte
     * and a command in its high byte. Zero is a type the part does not have. */
    static void SetEraseTypes(uint32_t first, uint32_t second, uint32_t third, uint32_t fourth)
    {
        SetDword(DWORD_ERASE_TYPES_1_AND_2, first | (second << 16U));
        SetDword(DWORD_ERASE_TYPES_3_AND_4, third | (fourth << 16U));
    }

    static void SetAddressBytes(uint32_t field)
    {
        SetDword(DWORD_ADDRESS_BYTES, (Dword(DWORD_ADDRESS_BYTES) & ~ADDRESS_BYTES_MASK) | field);
    }

    /* One more parameter header after those the SFDP has, for a table of
     * that ID. The table itself is not there: nothing should read it. */
    static void AddParameterHeader(uint8_t idLow, uint8_t idHigh)
    {
        auto& sfdp = SpiNorFlashPartFake::settings.sfdp;
        sfdp.at(SFDP_FURTHER_PARAMETER_HEADERS)++;
        const size_t at = AddressOfParameterHeader(sfdp.at(SFDP_FURTHER_PARAMETER_HEADERS));
        const std::array<uint8_t, PARAMETER_HEADER_BYTES> header = {idLow, 0x00, 0x01, 0x02, 0x40, 0x00, 0x00, idHigh};
        std::copy(header.begin(), header.end(), sfdp.begin() + static_cast<std::ptrdiff_t>(at));
    }

    /* Numbered from 0, which is the basic table's. */
    static size_t AddressOfParameterHeader(size_t number)
    {
        return SFDP_HEADER_BYTES + (number * PARAMETER_HEADER_BYTES);
    }

    static std::array<struct SolidSyslogSpiNorFlashBus, 5> TheFakesBusWithEachMemberMissingInTurn()
    {
        const struct SolidSyslogSpiNorFlashBus whole = *SpiNorFlashPartFake::Bus();
        std::array<struct SolidSyslogSpiNorFlashBus, 5> incomplete = {whole, whole, whole, whole, whole};
        incomplete[0].Select = nullptr;
        incomplete[1].Deselect = nullptr;
        incomplete[2].Write = nullptr;
        incomplete[3].Read = nullptr;
        incomplete[4].Milliseconds = nullptr;
        return incomplete;
    }
};

TEST_GROUP_BASE(SolidSyslogSpiNorFlash, SpiNorFlashTestBase)
{
    void setup() override
    {
        SetupPart();
    }

    void teardown() override
    {
        TeardownPart();
    }
};

// clang-format on

TEST(SolidSyslogSpiNorFlash, CreatesAFlashOverThePartItFinds)
{
    CreateFlash();
    CHECK_CREATED();
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogSpiNorFlash, HasBlocksOfThePartsSmallestErase)
{
    CreateFlash();
    LONGS_EQUAL(SECTOR_BYTES, SolidSyslogNorFlash_GetBlockSize(flash));
}

TEST(SolidSyslogSpiNorFlash, HasBlocksCoveringTheWholePart)
{
    CreateFlash();
    LONGS_EQUAL(SECTORS_IN_SIXTEEN_MEBIBYTES, SolidSyslogNorFlash_GetBlockCount(flash));
}

TEST(SolidSyslogSpiNorFlash, LeavesThePartDeselectedOnceCreated)
{
    CreateFlash();
    CHECK_FALSE(SpiNorFlashPartFake::IsSelected());
    LONGS_EQUAL(0, SpiNorFlashPartFake::TransfersWhileDeselected());
}

TEST(SolidSyslogSpiNorFlash, WritesNothingToThePartAsItStarts)
{
    CreateFlash();
    LONGS_EQUAL(0, SpiNorFlashPartFake::CountOf(WRITE_ENABLE));
    LONGS_EQUAL(0, SpiNorFlashPartFake::CountOf(WRITE_STATUS));
}

TEST(SolidSyslogSpiNorFlash, FindsTheSmallestEraseWhereverTheTableListsIt)
{
    SetEraseTypes(ERASE_64K_BY_D8, 0, 0, ERASE_4K_BY_20);
    CreateFlash();
    LONGS_EQUAL(SECTOR_BYTES, SolidSyslogNorFlash_GetBlockSize(flash));
}

TEST(SolidSyslogSpiNorFlash, HasLargerBlocksOnAPartWithNoSmallErase)
{
    SetEraseTypes(0, ERASE_64K_BY_D8, 0, 0);
    CreateFlash();
    LONGS_EQUAL(BLOCK_BYTES, SolidSyslogNorFlash_GetBlockSize(flash));
    LONGS_EQUAL(BLOCKS_IN_SIXTEEN_MEBIBYTES, SolidSyslogNorFlash_GetBlockCount(flash));
}

TEST(SolidSyslogSpiNorFlash, HasFewerBlocksOnASmallerPart)
{
    SetDword(DWORD_DENSITY, DENSITY_8_MEBIBYTES);
    CreateFlash();
    LONGS_EQUAL(SECTORS_IN_SIXTEEN_MEBIBYTES / 2, SolidSyslogNorFlash_GetBlockCount(flash));
}

TEST(SolidSyslogSpiNorFlash, ReadsADensityGivenAsAPowerOfTwo)
{
    SetDword(DWORD_DENSITY, DENSITY_AS_AN_EXPONENT | EXPONENT_OF_2_MEBIBYTES);
    CreateFlash();
    LONGS_EQUAL(SECTORS_IN_SIXTEEN_MEBIBYTES / 8, SolidSyslogNorFlash_GetBlockCount(flash));
}

TEST(SolidSyslogSpiNorFlash, TakesAPartOfExactlySixteenMebibytesGivenAsAPowerOfTwo)
{
    SetDword(DWORD_DENSITY, DENSITY_AS_AN_EXPONENT | EXPONENT_OF_16_MEBIBYTES);
    CreateFlash();
    LONGS_EQUAL(SECTORS_IN_SIXTEEN_MEBIBYTES, SolidSyslogNorFlash_GetBlockCount(flash));
}

TEST(SolidSyslogSpiNorFlash, TakesAPartThatAcceptsThreeByteAddressesOrFour)
{
    SetAddressBytes(THREE_OR_FOUR_ADDRESS_BYTES);
    CreateFlash();
    CHECK_CREATED();
}

TEST(SolidSyslogSpiNorFlash, ReadsTheTableFromWhereTheHeaderSaysItIs)
{
    auto& sfdp = SpiNorFlashPartFake::settings.sfdp;
    for (size_t i = 0; i < TABLE_BYTES; i++)
    {
        sfdp.at(ANOTHER_TABLE_ADDRESS + i) = sfdp.at(TABLE_ADDRESS + i);
        sfdp.at(TABLE_ADDRESS + i) = 0xFF;
    }
    sfdp.at(TABLE_POINTER) = ANOTHER_TABLE_ADDRESS;
    CreateFlash();
    LONGS_EQUAL(SECTOR_BYTES, SolidSyslogNorFlash_GetBlockSize(flash));
}

TEST(SolidSyslogSpiNorFlash, ReadsATableOfTheLengthJesd216FirstDefined)
{
    SpiNorFlashPartFake::settings.sfdp.at(TABLE_LENGTH_DWORDS) = DWORDS_IN_THE_FIRST_JESD216;
    CreateFlash();
    LONGS_EQUAL(SECTOR_BYTES, SolidSyslogNorFlash_GetBlockSize(flash));
}

TEST(SolidSyslogSpiNorFlash, WaitsForAPartThatIsBusyBeforeAskingItAnythingElse)
{
    SpiNorFlashPartFake::settings.busyAtPowerUpMilliseconds = 100;
    CreateFlash();
    CHECK_CREATED();
    LONGS_EQUAL(0, SpiNorFlashPartFake::CommandsWhileBusy());
}

TEST(SolidSyslogSpiNorFlash, WaitsNearlyThreeSecondsForAPartToBeReady)
{
    SpiNorFlashPartFake::settings.busyAtPowerUpMilliseconds = START_TIMEOUT_MILLISECONDS - 100;
    CreateFlash();
    CHECK_CREATED();
}

TEST(SolidSyslogSpiNorFlash, GivesUpOnAPartStillBusyAfterThreeSeconds)
{
    SpiNorFlashPartFake::settings.busyAtPowerUpMilliseconds = START_TIMEOUT_MILLISECONDS + 100;
    CreateFlash();
    CHECK_DID_NOT_START(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_TIMED_OUT);
}

TEST(SolidSyslogSpiNorFlash, AsksOnceMoreAfterTheTimeAllowedHasPassed)
{
    /* A task switched out for longer than the wait comes back to a part that
     * finished while it was away. */
    SpiNorFlashPartFake::settings.millisecondsPerReading = START_TIMEOUT_MILLISECONDS + 2000;
    SpiNorFlashPartFake::settings.busyAtPowerUpMilliseconds = START_TIMEOUT_MILLISECONDS + 4000;
    CreateFlash();
    CHECK_CREATED();
}

TEST(SolidSyslogSpiNorFlash, DoesNotStartWithNoPartOnTheBus)
{
    SpiNorFlashPartFake::settings.present = false;
    CreateFlash();
    CHECK_DID_NOT_START(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_ANSWER);
}

TEST(SolidSyslogSpiNorFlash, TakesALineHeldLowForAPartWithNoSfdp)
{
    /* A line held low reads as a status that says ready, so it is the
     * table of zeros after it that is refused. */
    SpiNorFlashPartFake::settings.lineHeldLow = true;
    CreateFlash();
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_SFDP);
}

TEST(SolidSyslogSpiNorFlash, DoesNotStartWhenAnyWriteOfTheBusFails)
{
    for (int call = 1; call <= TRANSFERS_IN_A_START; call++)
    {
        ErrorHandlerFake_Install(nullptr);
        SpiNorFlashPartFake::FailWriteAt(call);
        CreateFlash();
        CHECK_DID_NOT_START(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED);
        CHECK_FALSE(SpiNorFlashPartFake::IsSelected());
    }
}

TEST(SolidSyslogSpiNorFlash, DoesNotStartWhenAnyReadOfTheBusFails)
{
    for (int call = 1; call <= TRANSFERS_IN_A_START; call++)
    {
        ErrorHandlerFake_Install(nullptr);
        SpiNorFlashPartFake::FailReadAt(call);
        CreateFlash();
        CHECK_DID_NOT_START(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED);
        CHECK_FALSE(SpiNorFlashPartFake::IsSelected());
    }
}

TEST(SolidSyslogSpiNorFlash, StartsWhenTheBusFailsOnlyAfterTheStart)
{
    SpiNorFlashPartFake::FailWriteAt(TRANSFERS_IN_A_START + 1);
    SpiNorFlashPartFake::FailReadAt(TRANSFERS_IN_A_START + 1);
    CreateFlash();
    CHECK_CREATED();
}

TEST(SolidSyslogSpiNorFlash, RefusesAPartWithNoSfdp)
{
    SpiNorFlashPartFake::settings.answersSfdp = false;
    CreateFlash();
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_SFDP);
}

TEST(SolidSyslogSpiNorFlash, RefusesAPartWhoseSfdpSignatureIsWrongInItsLastByte)
{
    SpiNorFlashPartFake::settings.sfdp[3] = 'Q';
    CreateFlash();
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_SFDP);
}

TEST(SolidSyslogSpiNorFlash, RefusesAnSfdpOfAnotherMajorRevision)
{
    SpiNorFlashPartFake::settings.sfdp.at(SFDP_MAJOR_REVISION) = 2;
    CreateFlash();
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_SFDP_NOT_UNDERSTOOD);
}

TEST(SolidSyslogSpiNorFlash, RefusesAnSfdpWhoseFirstTableIsNotTheBasicOneByItsLowId)
{
    SpiNorFlashPartFake::settings.sfdp.at(TABLE_ID_LOW) = 0x84;
    CreateFlash();
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_SFDP_NOT_UNDERSTOOD);
}

TEST(SolidSyslogSpiNorFlash, RefusesAnSfdpWhoseFirstTableIsNotTheBasicOneByItsHighId)
{
    SpiNorFlashPartFake::settings.sfdp.at(TABLE_ID_HIGH) = 0xEF;
    CreateFlash();
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_SFDP_NOT_UNDERSTOOD);
}

TEST(SolidSyslogSpiNorFlash, RefusesABasicTableOfAnotherMajorRevision)
{
    SpiNorFlashPartFake::settings.sfdp.at(TABLE_MAJOR_REVISION) = 2;
    CreateFlash();
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_SFDP_NOT_UNDERSTOOD);
}

TEST(SolidSyslogSpiNorFlash, RefusesABasicTableShorterThanJesd216Defines)
{
    SpiNorFlashPartFake::settings.sfdp.at(TABLE_LENGTH_DWORDS) = DWORDS_IN_THE_FIRST_JESD216 - 1;
    CreateFlash();
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_SFDP_NOT_UNDERSTOOD);
}

TEST(SolidSyslogSpiNorFlash, RefusesATableThatListsNoErase)
{
    SetEraseTypes(0, 0, 0, 0);
    CreateFlash();
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_SFDP_NOT_UNDERSTOOD);
}

TEST(SolidSyslogSpiNorFlash, RefusesATableWhoseOnlyEraseIsLargerThanAnyPartItTakes)
{
    SetEraseTypes(ERASE_32M_BY_DC, 0, 0, 0);
    CreateFlash();
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_SFDP_NOT_UNDERSTOOD);
}

TEST(SolidSyslogSpiNorFlash, RefusesATableWhoseOnlyEraseIsLargerThanThePart)
{
    SetDword(DWORD_DENSITY, DENSITY_AS_AN_EXPONENT | EXPONENT_OF_32_KIBIBYTES);
    SetEraseTypes(ERASE_64K_BY_D8, 0, 0, 0);
    CreateFlash();
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_SFDP_NOT_UNDERSTOOD);
}

TEST(SolidSyslogSpiNorFlash, RefusesAPartOfMoreThanSixteenMebibytes)
{
    SetDword(DWORD_DENSITY, DENSITY_32_MEBIBYTES);
    CreateFlash();
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_PART_TOO_LARGE);
}

TEST(SolidSyslogSpiNorFlash, RefusesAPartOfMoreThanSixteenMebibytesGivenAsAPowerOfTwo)
{
    SetDword(DWORD_DENSITY, DENSITY_AS_AN_EXPONENT | EXPONENT_OF_32_MEBIBYTES);
    CreateFlash();
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_PART_TOO_LARGE);
}

TEST(SolidSyslogSpiNorFlash, RefusesAPartThatTakesOnlyFourByteAddresses)
{
    SetAddressBytes(FOUR_ADDRESS_BYTES_ONLY);
    CreateFlash();
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_FOUR_BYTE_ADDRESSES_ONLY);
}

TEST(SolidSyslogSpiNorFlash, RefusesAnAddressWidthJesd216Reserves)
{
    SetAddressBytes(ADDRESS_BYTES_RESERVED);
    CreateFlash();
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_FOUR_BYTE_ADDRESSES_ONLY);
}

TEST(SolidSyslogSpiNorFlash, RefusesAPartWhoseSfdpHasASectorMap)
{
    AddParameterHeader(SECTOR_MAP_ID_LOW, JEDEC_TABLE_ID_HIGH);
    CreateFlash();
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_HAS_SECTOR_MAP);
}

TEST(SolidSyslogSpiNorFlash, FindsASectorMapAfterOtherParameterHeaders)
{
    AddParameterHeader(FOUR_BYTE_ADDRESS_TABLE_ID_LOW, JEDEC_TABLE_ID_HIGH);
    AddParameterHeader(SECTOR_MAP_ID_LOW, JEDEC_TABLE_ID_HIGH);
    CreateFlash();
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_HAS_SECTOR_MAP);
}

TEST(SolidSyslogSpiNorFlash, TakesAPartWhoseFurtherParameterHeadersAreNotOfASectorMap)
{
    AddParameterHeader(FOUR_BYTE_ADDRESS_TABLE_ID_LOW, JEDEC_TABLE_ID_HIGH);
    AddParameterHeader(STATUS_AND_CONTROL_TABLE_ID_LOW, JEDEC_TABLE_ID_HIGH);
    CreateFlash();
    CHECK_CREATED();
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogSpiNorFlash, TellsASectorMapByBothBytesOfItsId)
{
    AddParameterHeader(SECTOR_MAP_ID_LOW, A_MAKERS_TABLE_ID_HIGH);
    CreateFlash();
    CHECK_CREATED();
}

TEST(SolidSyslogSpiNorFlash, ReadsEachFurtherParameterHeaderAndNoTableButTheBasicOne)
{
    AddParameterHeader(FOUR_BYTE_ADDRESS_TABLE_ID_LOW, JEDEC_TABLE_ID_HIGH);
    AddParameterHeader(STATUS_AND_CONTROL_TABLE_ID_LOW, JEDEC_TABLE_ID_HIGH);
    CreateFlash();
    const auto& reads = SpiNorFlashPartFake::SfdpReads();
    LONGS_EQUAL(4, reads.size());
    CHECK_SFDP_READ(0, FIRST_SFDP_READ_BYTES, reads[0]);
    CHECK_SFDP_READ(AddressOfParameterHeader(1), PARAMETER_HEADER_BYTES, reads[1]);
    CHECK_SFDP_READ(AddressOfParameterHeader(2), PARAMETER_HEADER_BYTES, reads[2]);
    CHECK_SFDP_READ(TABLE_ADDRESS, TABLE_BYTES_READ, reads[3]);
}

TEST(SolidSyslogSpiNorFlash, ReadsNoMoreOfASectorMapThanItsParameterHeader)
{
    AddParameterHeader(SECTOR_MAP_ID_LOW, JEDEC_TABLE_ID_HIGH);
    AddParameterHeader(FOUR_BYTE_ADDRESS_TABLE_ID_LOW, JEDEC_TABLE_ID_HIGH);
    CreateFlash();
    const auto& reads = SpiNorFlashPartFake::SfdpReads();
    LONGS_EQUAL(2, reads.size());
    CHECK_SFDP_READ(0, FIRST_SFDP_READ_BYTES, reads[0]);
    CHECK_SFDP_READ(AddressOfParameterHeader(1), PARAMETER_HEADER_BYTES, reads[1]);
}

TEST(SolidSyslogSpiNorFlash, ReadsNoFurtherParameterHeaderOfAPartThatHasNone)
{
    CreateFlash();
    LONGS_EQUAL(2, SpiNorFlashPartFake::CountOf(READ_SFDP));
}

TEST(SolidSyslogSpiNorFlash, DoesNotStartWhenTheWriteAskingForAFurtherParameterHeaderFails)
{
    AddParameterHeader(FOUR_BYTE_ADDRESS_TABLE_ID_LOW, JEDEC_TABLE_ID_HIGH);
    SpiNorFlashPartFake::FailWriteAt(TRANSFERS_BEFORE_A_FURTHER_PARAMETER_HEADER + 1);
    CreateFlash();
    CHECK_DID_NOT_START(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED);
}

TEST(SolidSyslogSpiNorFlash, DoesNotStartWhenTheReadOfAFurtherParameterHeaderFails)
{
    AddParameterHeader(FOUR_BYTE_ADDRESS_TABLE_ID_LOW, JEDEC_TABLE_ID_HIGH);
    SpiNorFlashPartFake::FailReadAt(TRANSFERS_BEFORE_A_FURTHER_PARAMETER_HEADER + 1);
    CreateFlash();
    CHECK_DID_NOT_START(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED);
    /* The parameter header was asked for, none of it came, and the table
     * was not asked for after it. */
    const auto& reads = SpiNorFlashPartFake::SfdpReads();
    LONGS_EQUAL(2, reads.size());
    CHECK_SFDP_READ(AddressOfParameterHeader(1), 0, reads[1]);
}

TEST(SolidSyslogSpiNorFlash, TakesAPartWhateverTheOtherBitsOfItsStatusSay)
{
    /* Protection is the part's owner's business: it is neither looked for
     * nor refused. */
    SpiNorFlashPartFake::settings.status = STATUS_BITS_THE_PART_DOES_NOT_SET_ITSELF;
    CreateFlash();
    CHECK_CREATED();
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogSpiNorFlash, LeavesTheStatusOfAPartAsItFindsIt)
{
    SpiNorFlashPartFake::settings.status = STATUS_BITS_THE_PART_DOES_NOT_SET_ITSELF;
    CreateFlash();
    LONGS_EQUAL(0, SpiNorFlashPartFake::CountOf(WRITE_ENABLE));
    LONGS_EQUAL(0, SpiNorFlashPartFake::CountOf(WRITE_STATUS));
}

TEST(SolidSyslogSpiNorFlash, AsksNothingOfThePartOnceItHasItsTable)
{
    CreateFlash();
    LONGS_EQUAL(COMMANDS_IN_A_START, SpiNorFlashPartFake::Commands().size());
}

TEST(SolidSyslogSpiNorFlash, RefusesANullConfig)
{
    flash = SolidSyslogSpiNorFlash_Create(nullptr);
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NULL_CONFIG);
}

TEST(SolidSyslogSpiNorFlash, RefusesAConfigWithNoBus)
{
    const struct SolidSyslogSpiNorFlashConfig config = {nullptr};
    flash = SolidSyslogSpiNorFlash_Create(&config);
    CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NULL_BUS);
}

TEST(SolidSyslogSpiNorFlash, RefusesABusWithAnyMemberNotFilled)
{
    for (auto& bus : TheFakesBusWithEachMemberMissingInTurn())
    {
        ErrorHandlerFake_Install(nullptr);
        const struct SolidSyslogSpiNorFlashConfig config = {&bus};
        flash = SolidSyslogSpiNorFlash_Create(&config);
        CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_INCOMPLETE);
    }
}

TEST(SolidSyslogSpiNorFlash, AsksNothingOfABusItRefuses)
{
    /* Each is the fake's own bus but for the member missing, so a call of
     * any member that is there would show. */
    for (auto& bus : TheFakesBusWithEachMemberMissingInTurn())
    {
        const struct SolidSyslogSpiNorFlashConfig config = {&bus};
        flash = SolidSyslogSpiNorFlash_Create(&config);
        CHECK_FALSE(SpiNorFlashPartFake::IsSelected());
        LONGS_EQUAL(0, SpiNorFlashPartFake::Commands().size());
        LONGS_EQUAL(0, SpiNorFlashPartFake::TransfersWhileDeselected());
        LONGS_EQUAL(0, SpiNorFlashPartFake::Milliseconds());
    }
}

TEST(SolidSyslogSpiNorFlash, IsTheNullFlashOnceDestroyed)
{
    CreateFlash();
    struct SolidSyslogNorFlash* destroyed = flash;
    DestroyFlash();
    LONGS_EQUAL(0, SolidSyslogNorFlash_GetBlockSize(destroyed));
    LONGS_EQUAL(0, SolidSyslogNorFlash_GetBlockCount(destroyed));
}

TEST(SolidSyslogSpiNorFlash, ReportsADestroyOfSomethingItDidNotCreate)
{
    struct SolidSyslogNorFlashDummy
    {
        void* members[5];
    } notAFlash = {};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- a handle the pool never issued
    SolidSyslogSpiNorFlash_Destroy(reinterpret_cast<struct SolidSyslogNorFlash*>(&notAFlash));

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogSpiNorFlashErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogSpiNorFlash, ReportsASecondDestroyOfTheSameFlash)
{
    CreateFlash();
    struct SolidSyslogNorFlash* destroyed = flash;
    DestroyFlash();
    SolidSyslogSpiNorFlash_Destroy(destroyed);
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogSpiNorFlashErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogSpiNorFlash, NamesEveryDetailItReports)
{
    STRCMP_EQUAL("NO_SFDP", SolidSyslogSpiNorFlashErrors_Name(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_SFDP));
    for (int32_t detail = 0; detail < SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_MAX; detail++)
    {
        CHECK_TRUE(SolidSyslogSpiNorFlashErrors_Name(detail) != nullptr);
    }
    POINTERS_EQUAL(nullptr, SolidSyslogSpiNorFlashErrors_Name(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_MAX));
}

TEST(SolidSyslogSpiNorFlash, NamesItselfAsTheSourceOfWhatItReports)
{
    STRCMP_EQUAL("SpiNorFlash", SolidSyslogSpiNorFlashErrorSource.Name);
}

// clang-format off
TEST_GROUP_BASE(SolidSyslogSpiNorFlashPool, SpiNorFlashTestBase)
{
    std::array<struct SolidSyslogNorFlash*, SOLIDSYSLOG_NOR_FLASH_POOL_SIZE> pooled = {};

    void setup() override
    {
        SetupPart();
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogSpiNorFlash_Destroy(handle);
            }
        }
        TeardownPart();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            CreateFlash();
            slot = flash;
            flash = nullptr;
        }
    }
};

// clang-format on

TEST(SolidSyslogSpiNorFlashPool, FillsWithoutAReport)
{
    FillPool();
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogSpiNorFlashPool, ReportsExhaustionAndAnswersTheNullFlash)
{
    FillPool();
    CreateFlash();
    CHECK_NOT_CREATED();
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogSpiNorFlashErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogSpiNorFlashPool, AsksNothingOfThePartWhenThePoolIsExhausted)
{
    FillPool();
    const size_t commands = SpiNorFlashPartFake::Commands().size();
    CreateFlash();
    LONGS_EQUAL(commands, SpiNorFlashPartFake::Commands().size());
}

TEST(SolidSyslogSpiNorFlashPool, GivesBackTheSlotOfAPartItRefuses)
{
    SpiNorFlashPartFake::settings.answersSfdp = false;
    CreateFlash();
    CHECK_NOT_CREATED();
    SpiNorFlashPartFake::settings.answersSfdp = true;
    FillPool();
    /* Every slot: one the refusal kept would leave the last without. */
    for (const auto* handle : pooled)
    {
        CHECK_IS_A_FLASH(handle);
    }
}

TEST(SolidSyslogSpiNorFlashPool, HasASlotAgainOnceAFlashIsDestroyed)
{
    FillPool();
    SolidSyslogSpiNorFlash_Destroy(pooled[0]);
    pooled[0] = nullptr;
    CreateFlash();
    CHECK_CREATED();
}

enum
{
    PAGE_PROGRAM = 0x02,
    READ_DATA = 0x03,
    READ_STATUS = 0x05,
    SECTOR_ERASE = 0x20,
    BLOCK_ERASE = 0xD8,

    PAGE_BYTES = 256,
    A_BLOCK = 0x42,
    START_OF_A_BLOCK = A_BLOCK * SECTOR_BYTES,
    AN_OFFSET = 0x345,
    LAST_BYTE_OF_A_PAGE = 0x2FF,
    LAST_BLOCK = SECTORS_IN_SIXTEEN_MEBIBYTES - 1,
    THREE_PAGES_AND_A_LITTLE = (3 * PAGE_BYTES) + 10,
    MORE_THAN_A_PAGE = PAGE_BYTES + 44,

    /* What an erase and a page program are allowed, on any part. */
    ERASE_TIMEOUT = 3000,
    PROGRAM_TIMEOUT = 20,
    FAR_LONGER_THAN_ANY_TIMEOUT = 100000,

    /* The transfers each makes of a part that does as it is asked. A program
     * here is of one page. */
    WRITES_IN_A_READ = 2,
    READS_IN_A_READ = 2,
    WRITES_IN_A_PROGRAM = 5,
    READS_IN_A_PROGRAM = 2,
    WRITES_IN_AN_ERASE = 4,
    READS_IN_AN_ERASE = 2
};

/* Where a table of JESD216A or later gives the part's page size. */
static const size_t DWORD_PAGE_SIZE = 11;
static const uint32_t PAGE_SIZE_FIELD = 0x000000F0U;
static const uint32_t PAGE_SIZE_OF_512 = 0x00000090U;
/* Near enough to where the count of milliseconds wraps that an erase begun
 * then is still going after it. */
static const uint32_t A_LITTLE_BEFORE_THE_WRAP = UINT32_MAX - 100U;

#define CHECK_OPERATION_FAILED(detail, succeeded) \
    {                                             \
        CHECK_FALSE(succeeded);                   \
        CHECK_IO_FAILURE_REPORTED(detail);        \
    }

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
TEST_GROUP_BASE(SolidSyslogSpiNorFlashOperations, SpiNorFlashTestBase)
{
    std::vector<uint8_t> buffer;
    size_t commandsInTheStart = 0;

    void setup() override
    {
        SetupPart();
    }

    void teardown() override
    {
        TeardownPart();
        buffer = std::vector<uint8_t>();
    }

    /* Creates the flash over the part as the test has arranged it, and
     * forgets what creating it reported and sent. */
    void Start()
    {
        CreateFlash();
        CHECK_CREATED();
        ErrorHandlerFake_Install(nullptr);
        commandsInTheStart = SpiNorFlashPartFake::Commands().size();
    }

    /* The first byte of each command the part has taken since Start. */
    [[nodiscard]] std::vector<uint8_t> CommandsSinceTheStart() const
    {
        const std::vector<uint8_t>& commands = SpiNorFlashPartFake::Commands();
        return {commands.begin() + static_cast<std::ptrdiff_t>(commandsInTheStart), commands.end()};
    }

    /* The first of those, where a wait went on to ask the status many times. */
    [[nodiscard]] std::vector<uint8_t> FirstCommandsSinceTheStart(size_t count) const
    {
        std::vector<uint8_t> commands = CommandsSinceTheStart();
        commands.resize(std::min(count, commands.size()));
        return commands;
    }

    /* A blank part and no flash, for a test that starts more than one. */
    void StartOver()
    {
        TeardownPart();
        SetupPart();
    }

    [[nodiscard]] bool Read(size_t block, size_t offset, size_t count)
    {
        buffer.assign(count, 0);
        return SolidSyslogNorFlash_Read(flash, block, offset, buffer.data(), count);
    }

    [[nodiscard]] bool Program(size_t block, size_t offset, const std::vector<uint8_t>& bytes) const
    {
        return SolidSyslogNorFlash_Program(flash, block, offset, bytes.data(), bytes.size());
    }

    [[nodiscard]] bool Erase(size_t block) const
    {
        return SolidSyslogNorFlash_Erase(flash, block);
    }

    static void LetTimePass(uint32_t milliseconds)
    {
        SolidSyslogSpiNorFlashBus* bus = SpiNorFlashPartFake::Bus();
        for (uint32_t i = 0; i < milliseconds; i++)
        {
            (void) bus->Milliseconds(bus);
        }
    }
};

// clang-format on

TEST(SolidSyslogSpiNorFlashOperations, ReadsTheBytesAtAnOffsetInABlock)
{
    const std::vector<uint8_t> held = {0x11, 0x22, 0x33, 0x44, 0x55};
    SpiNorFlashPartFake::Store(START_OF_A_BLOCK + AN_OFFSET, held);
    Start();
    CHECK_TRUE(Read(A_BLOCK, AN_OFFSET, held.size()));
    CHECK_BYTES_EQUAL(held, buffer);
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogSpiNorFlashOperations, ReadsAWholeBlockWithOneCommand)
{
    const std::vector<uint8_t> held = BytesOfPattern(3, SECTOR_BYTES);
    SpiNorFlashPartFake::Store(START_OF_A_BLOCK, held);
    Start();
    CHECK_TRUE(Read(A_BLOCK, 0, SECTOR_BYTES));
    CHECK_BYTES_EQUAL(held, buffer);
    LONGS_EQUAL(1, SpiNorFlashPartFake::CountOf(READ_DATA));
}

TEST(SolidSyslogSpiNorFlashOperations, ReadsTheLastByteOfThePart)
{
    SpiNorFlashPartFake::Store((SECTORS_IN_SIXTEEN_MEBIBYTES * SECTOR_BYTES) - 1, {0x5A});
    Start();
    CHECK_TRUE(Read(LAST_BLOCK, SECTOR_BYTES - 1, 1));
    BYTES_EQUAL(0x5A, buffer[0]);
}

TEST(SolidSyslogSpiNorFlashOperations, LeavesThePartDeselectedAfterARead)
{
    Start();
    (void) Read(A_BLOCK, AN_OFFSET, 4);
    CHECK_FALSE(SpiNorFlashPartFake::IsSelected());
    LONGS_EQUAL(0, SpiNorFlashPartFake::TransfersWhileDeselected());
}

TEST(SolidSyslogSpiNorFlashOperations, AsksThePartsStatusOnceItHasTheDataAndNotBefore)
{
    Start();
    CHECK_TRUE(Read(A_BLOCK, AN_OFFSET, 4));
    CHECK_BYTES_EQUAL(std::vector<uint8_t>({READ_DATA, READ_STATUS}), CommandsSinceTheStart());
}

TEST(SolidSyslogSpiNorFlashOperations, FailsAReadOfAPartThatIsBusy)
{
    Start();
    SpiNorFlashPartFake::settings.staysBusy = true;
    CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_TIMED_OUT, Read(A_BLOCK, AN_OFFSET, 4));
}

TEST(SolidSyslogSpiNorFlashOperations, FailsAReadOfAPartThatHasStoppedAnswering)
{
    Start();
    SpiNorFlashPartFake::settings.present = false;
    CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_ANSWER, Read(A_BLOCK, AN_OFFSET, 4));
}

TEST(SolidSyslogSpiNorFlashOperations, FailsAReadOfAPartThatStopsAnsweringOnceItHasTheCommand)
{
    Start();
    SpiNorFlashPartFake::GoQuietAfter(1);
    CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_ANSWER, Read(A_BLOCK, AN_OFFSET, 4));
}

TEST(SolidSyslogSpiNorFlashOperations, FailsAReadWhenAnyWriteOfTheBusFails)
{
    for (int call = 1; call <= WRITES_IN_A_READ; call++)
    {
        StartOver();
        Start();
        SpiNorFlashPartFake::FailWriteAt(call);
        CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED, Read(A_BLOCK, AN_OFFSET, 4));
        CHECK_FALSE(SpiNorFlashPartFake::IsSelected());
    }
}

TEST(SolidSyslogSpiNorFlashOperations, FailsAReadWhenAnyReadOfTheBusFails)
{
    for (int call = 1; call <= READS_IN_A_READ; call++)
    {
        StartOver();
        Start();
        SpiNorFlashPartFake::FailReadAt(call);
        CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED, Read(A_BLOCK, AN_OFFSET, 4));
        CHECK_FALSE(SpiNorFlashPartFake::IsSelected());
    }
}

TEST(SolidSyslogSpiNorFlashOperations, ProgramsBytesAtAnOffsetInABlock)
{
    const std::vector<uint8_t> bytes = {0x12, 0x34, 0x56};
    Start();
    CHECK_TRUE(Program(A_BLOCK, AN_OFFSET, bytes));
    CHECK_BYTES_EQUAL(bytes, SpiNorFlashPartFake::Stored(START_OF_A_BLOCK + AN_OFFSET, bytes.size()));
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogSpiNorFlashOperations, ProgramsAWholePageWithOneCommand)
{
    const std::vector<uint8_t> bytes = BytesOfPattern(1, PAGE_BYTES);
    Start();
    CHECK_TRUE(Program(A_BLOCK, PAGE_BYTES, bytes));
    CHECK_BYTES_EQUAL(bytes, SpiNorFlashPartFake::Stored(START_OF_A_BLOCK + PAGE_BYTES, bytes.size()));
    LONGS_EQUAL(1, SpiNorFlashPartFake::Programs().size());
}

TEST(SolidSyslogSpiNorFlashOperations, SplitsAProgramWhereItCrossesIntoTheNextPage)
{
    const std::vector<uint8_t> bytes = {0x01, 0x02, 0x03};
    Start();
    CHECK_TRUE(Program(A_BLOCK, LAST_BYTE_OF_A_PAGE, bytes));
    CHECK_BYTES_EQUAL(bytes, SpiNorFlashPartFake::Stored(START_OF_A_BLOCK + LAST_BYTE_OF_A_PAGE, bytes.size()));
    LONGS_EQUAL(2, SpiNorFlashPartFake::Programs().size());
    LONGS_EQUAL(START_OF_A_BLOCK + LAST_BYTE_OF_A_PAGE, SpiNorFlashPartFake::Programs()[0].address);
    LONGS_EQUAL(1, SpiNorFlashPartFake::Programs()[0].length);
    LONGS_EQUAL(START_OF_A_BLOCK + LAST_BYTE_OF_A_PAGE + 1, SpiNorFlashPartFake::Programs()[1].address);
    LONGS_EQUAL(2, SpiNorFlashPartFake::Programs()[1].length);
}

TEST(SolidSyslogSpiNorFlashOperations, ProgramsASpanOfManyPages)
{
    const std::vector<uint8_t> bytes = BytesOfPattern(7, THREE_PAGES_AND_A_LITTLE);
    Start();
    CHECK_TRUE(Program(A_BLOCK, LAST_BYTE_OF_A_PAGE, bytes));
    CHECK_BYTES_EQUAL(bytes, SpiNorFlashPartFake::Stored(START_OF_A_BLOCK + LAST_BYTE_OF_A_PAGE, bytes.size()));
    LONGS_EQUAL(5, SpiNorFlashPartFake::Programs().size());
}

TEST(SolidSyslogSpiNorFlashOperations, ProgramsAWholeBlock)
{
    const std::vector<uint8_t> bytes = BytesOfPattern(9, SECTOR_BYTES);
    Start();
    CHECK_TRUE(Program(LAST_BLOCK, 0, bytes));
    CHECK_BYTES_EQUAL(bytes, SpiNorFlashPartFake::Stored(LAST_BLOCK * SECTOR_BYTES, bytes.size()));
    LONGS_EQUAL(SECTOR_BYTES / PAGE_BYTES, SpiNorFlashPartFake::Programs().size());
}

TEST(SolidSyslogSpiNorFlashOperations, EnablesWritingBeforeEveryPageItPrograms)
{
    Start();
    CHECK_TRUE(Program(A_BLOCK, LAST_BYTE_OF_A_PAGE, {0x01, 0x02}));
    LONGS_EQUAL(2, SpiNorFlashPartFake::CountOf(WRITE_ENABLE));
}

TEST(SolidSyslogSpiNorFlashOperations, AsksNothingBeforeTheEnableOfAProgram)
{
    /* The status asked after the enable says whether it took and whether
     * the part is ready, so nothing is asked before it. */
    Start();
    CHECK_TRUE(Program(A_BLOCK, AN_OFFSET, {0x00}));
    CHECK_BYTES_EQUAL(std::vector<uint8_t>({WRITE_ENABLE, READ_STATUS, PAGE_PROGRAM}), FirstCommandsSinceTheStart(3));
}

TEST(SolidSyslogSpiNorFlashOperations, WaitsForOnePageToFinishBeforeItProgramsTheNext)
{
    Start();
    CHECK_TRUE(Program(A_BLOCK, LAST_BYTE_OF_A_PAGE, {0x01, 0x02}));
    LONGS_EQUAL(0, SpiNorFlashPartFake::CommandsWhileBusy());
}

TEST(SolidSyslogSpiNorFlashOperations, IsNoLongerBusyWhenAProgramReturns)
{
    Start();
    CHECK_TRUE(Program(A_BLOCK, AN_OFFSET, {0x00}));
    CHECK_FALSE(SpiNorFlashPartFake::IsBusy());
}

TEST(SolidSyslogSpiNorFlashOperations, ProgramsNothingWhenGivenNothing)
{
    Start();
    CHECK_TRUE(Program(A_BLOCK, AN_OFFSET, {}));
    LONGS_EQUAL(0, SpiNorFlashPartFake::CountOf(WRITE_ENABLE));
}

TEST(SolidSyslogSpiNorFlashOperations, SplitsAProgramBy256OnAPartWhoseTableGivesLargerPages)
{
    /* A page of 512 takes a program of either half of it, so the split
     * that suits a page of 256 suits it too, and the table is not asked. */
    SetDword(DWORD_PAGE_SIZE, (Dword(DWORD_PAGE_SIZE) & ~PAGE_SIZE_FIELD) | PAGE_SIZE_OF_512);
    SpiNorFlashPartFake::settings.pageBytes = 2 * PAGE_BYTES;
    const std::vector<uint8_t> bytes = BytesOfPattern(1, MORE_THAN_A_PAGE);
    Start();
    CHECK_TRUE(Program(A_BLOCK, 0, bytes));
    CHECK_BYTES_EQUAL(bytes, SpiNorFlashPartFake::Stored(START_OF_A_BLOCK, bytes.size()));
    LONGS_EQUAL(2, SpiNorFlashPartFake::Programs().size());
}

TEST(SolidSyslogSpiNorFlashOperations, SendsNoProgramToAPartThatDidNotEnableWriting)
{
    Start();
    SpiNorFlashPartFake::settings.ignoresWriteEnable = true;
    CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_WRITE_REFUSED, Program(A_BLOCK, AN_OFFSET, {0x00}));
    LONGS_EQUAL(0, SpiNorFlashPartFake::CountOf(PAGE_PROGRAM));
}

TEST(SolidSyslogSpiNorFlashOperations, AllowsAPageProgramTwentyMilliseconds)
{
    /* The fixture's own table says 5 ms at the longest. It is not asked. */
    Start();
    SpiNorFlashPartFake::settings.programMilliseconds = PROGRAM_TIMEOUT - 2;
    CHECK_TRUE(Program(A_BLOCK, AN_OFFSET, {0x00}));
}

TEST(SolidSyslogSpiNorFlashOperations, GivesUpOnAPageProgramAfterTwentyMilliseconds)
{
    Start();
    SpiNorFlashPartFake::settings.programMilliseconds = PROGRAM_TIMEOUT + 5;
    CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_TIMED_OUT, Program(A_BLOCK, AN_OFFSET, {0x00}));
}

TEST(SolidSyslogSpiNorFlashOperations, StopsAtThePageThatFails)
{
    Start();
    SpiNorFlashPartFake::settings.programMilliseconds = FAR_LONGER_THAN_ANY_TIMEOUT;
    CHECK_FALSE(Program(A_BLOCK, LAST_BYTE_OF_A_PAGE, {0x01, 0x02}));
    LONGS_EQUAL(1, SpiNorFlashPartFake::CountOf(PAGE_PROGRAM));
    /* The second page was not sent to a part too busy to take it, either. */
    LONGS_EQUAL(0, SpiNorFlashPartFake::CommandsWhileBusy());
}

TEST(SolidSyslogSpiNorFlashOperations, SendsNoProgramToAPartLeftBusyByOneThatOutlastedItsTime)
{
    Start();
    SpiNorFlashPartFake::settings.programMilliseconds = FAR_LONGER_THAN_ANY_TIMEOUT;
    (void) Program(A_BLOCK, AN_OFFSET, {0x00});
    ErrorHandlerFake_Install(nullptr);
    CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_TIMED_OUT, Program(A_BLOCK, AN_OFFSET + 1, {0x00}));
    /* The enable reached the busy part, which ignored it, and nothing was
     * sent after it. */
    LONGS_EQUAL(1, SpiNorFlashPartFake::CommandsWhileBusy());
}

TEST(SolidSyslogSpiNorFlashOperations, SendsNoEraseToAPartLeftBusyByAProgramThatOutlastedItsTime)
{
    Start();
    SpiNorFlashPartFake::settings.programMilliseconds = FAR_LONGER_THAN_ANY_TIMEOUT;
    (void) Program(A_BLOCK, AN_OFFSET, {0x00});
    ErrorHandlerFake_Install(nullptr);
    CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_TIMED_OUT, Erase(A_BLOCK));
    LONGS_EQUAL(1, SpiNorFlashPartFake::CommandsWhileBusy());
}

TEST(SolidSyslogSpiNorFlashOperations, ProgramsAgainOnceThePartHasFinishedWhatOutlastedItsTime)
{
    Start();
    SpiNorFlashPartFake::settings.programMilliseconds = 1000;
    (void) Program(A_BLOCK, AN_OFFSET, {0x00});
    LetTimePass(1000);
    SpiNorFlashPartFake::settings.programMilliseconds = 1;
    CHECK_TRUE(Program(A_BLOCK, AN_OFFSET + 1, {0x00}));
}

TEST(SolidSyslogSpiNorFlashOperations, FailsAProgramWhenThePartStopsAnsweringBeforeWritingIsEnabled)
{
    Start();
    /* It takes the enable, and answers nothing after. */
    SpiNorFlashPartFake::GoQuietAfter(1);
    CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_ANSWER, Program(A_BLOCK, AN_OFFSET, {0x00}));
    LONGS_EQUAL(0, SpiNorFlashPartFake::CountOf(PAGE_PROGRAM));
}

TEST(SolidSyslogSpiNorFlashOperations, FailsAProgramWhenThePartStopsAnsweringWhileItPrograms)
{
    Start();
    /* Writing enabled, its status, and the program itself. */
    SpiNorFlashPartFake::GoQuietAfter(3);
    CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_ANSWER, Program(A_BLOCK, AN_OFFSET, {0x00}));
}

TEST(SolidSyslogSpiNorFlashOperations, FailsAProgramWhenAnyWriteOfTheBusFails)
{
    for (int call = 1; call <= WRITES_IN_A_PROGRAM; call++)
    {
        StartOver();
        Start();
        SpiNorFlashPartFake::FailWriteAt(call);
        CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED, Program(A_BLOCK, AN_OFFSET, {0x00}));
        CHECK_FALSE(SpiNorFlashPartFake::IsSelected());
    }
}

TEST(SolidSyslogSpiNorFlashOperations, FailsAProgramWhenAnyReadOfTheBusFails)
{
    for (int call = 1; call <= READS_IN_A_PROGRAM; call++)
    {
        StartOver();
        Start();
        SpiNorFlashPartFake::FailReadAt(call);
        CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED, Program(A_BLOCK, AN_OFFSET, {0x00}));
        CHECK_FALSE(SpiNorFlashPartFake::IsSelected());
    }
}

TEST(SolidSyslogSpiNorFlashOperations, ErasesABlock)
{
    SpiNorFlashPartFake::Store(START_OF_A_BLOCK, BytesOfPattern(0, SECTOR_BYTES));
    Start();
    CHECK_TRUE(Erase(A_BLOCK));
    CHECK_BYTES_EQUAL(
        std::vector<uint8_t>(SECTOR_BYTES, 0xFF),
        SpiNorFlashPartFake::Stored(START_OF_A_BLOCK, SECTOR_BYTES)
    );
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogSpiNorFlashOperations, ErasesOneBlockAndNoOther)
{
    Start();
    CHECK_TRUE(Erase(A_BLOCK));
    LONGS_EQUAL(1, SpiNorFlashPartFake::Erases().size());
    LONGS_EQUAL(START_OF_A_BLOCK, SpiNorFlashPartFake::Erases()[0]);
}

TEST(SolidSyslogSpiNorFlashOperations, AsksNothingBeforeTheEnableOfAnErase)
{
    Start();
    CHECK_TRUE(Erase(A_BLOCK));
    CHECK_BYTES_EQUAL(std::vector<uint8_t>({WRITE_ENABLE, READ_STATUS, SECTOR_ERASE}), FirstCommandsSinceTheStart(3));
}

TEST(SolidSyslogSpiNorFlashOperations, ErasesByTheCommandTheTableGivesForItsSmallestErase)
{
    SetEraseTypes(0, ERASE_64K_BY_D8, 0, 0);
    SpiNorFlashPartFake::settings.eraseCommand = BLOCK_ERASE;
    SpiNorFlashPartFake::settings.eraseBytes = BLOCK_BYTES;
    Start();
    CHECK_TRUE(Erase(2));
    LONGS_EQUAL(1, SpiNorFlashPartFake::Erases().size());
    LONGS_EQUAL(2 * BLOCK_BYTES, SpiNorFlashPartFake::Erases()[0]);
    LONGS_EQUAL(0, SpiNorFlashPartFake::CountOf(SECTOR_ERASE));
}

TEST(SolidSyslogSpiNorFlashOperations, IsNoLongerBusyWhenAnEraseReturns)
{
    Start();
    CHECK_TRUE(Erase(A_BLOCK));
    CHECK_FALSE(SpiNorFlashPartFake::IsBusy());
}

TEST(SolidSyslogSpiNorFlashOperations, ErasesTheLastBlockOfThePart)
{
    Start();
    CHECK_TRUE(Erase(LAST_BLOCK));
}

TEST(SolidSyslogSpiNorFlashOperations, SendsNoEraseToAPartThatDidNotEnableWriting)
{
    Start();
    SpiNorFlashPartFake::settings.ignoresWriteEnable = true;
    CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_WRITE_REFUSED, Erase(A_BLOCK));
    LONGS_EQUAL(0, SpiNorFlashPartFake::CountOf(SECTOR_ERASE));
}

TEST(SolidSyslogSpiNorFlashOperations, AllowsAnEraseItsTimeAcrossAWrapOfTheCountOfMilliseconds)
{
    SpiNorFlashPartFake::StartClockAt(A_LITTLE_BEFORE_THE_WRAP);
    Start();
    SpiNorFlashPartFake::settings.eraseMilliseconds = ERASE_TIMEOUT - 10;
    CHECK_TRUE(Erase(A_BLOCK));
}

TEST(SolidSyslogSpiNorFlashOperations, GivesUpOnAnEraseThatOutlastsItsTimeAcrossAWrapOfTheCountOfMilliseconds)
{
    SpiNorFlashPartFake::StartClockAt(A_LITTLE_BEFORE_THE_WRAP);
    Start();
    SpiNorFlashPartFake::settings.eraseMilliseconds = ERASE_TIMEOUT + 10;
    CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_TIMED_OUT, Erase(A_BLOCK));
}

TEST(SolidSyslogSpiNorFlashOperations, AllowsAnEraseThreeSeconds)
{
    /* The fixture's own table says 896 ms at the longest. It is not asked. */
    Start();
    SpiNorFlashPartFake::settings.eraseMilliseconds = ERASE_TIMEOUT - 10;
    CHECK_TRUE(Erase(A_BLOCK));
}

TEST(SolidSyslogSpiNorFlashOperations, GivesUpOnAnEraseAfterThreeSeconds)
{
    Start();
    SpiNorFlashPartFake::settings.eraseMilliseconds = ERASE_TIMEOUT + 10;
    CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_TIMED_OUT, Erase(A_BLOCK));
}

TEST(SolidSyslogSpiNorFlashOperations, FailsAReadOfAPartLeftBusyByAnEraseThatOutlastedItsTime)
{
    Start();
    SpiNorFlashPartFake::settings.eraseMilliseconds = FAR_LONGER_THAN_ANY_TIMEOUT;
    (void) Erase(A_BLOCK);
    ErrorHandlerFake_Install(nullptr);
    CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_TIMED_OUT, Read(A_BLOCK, AN_OFFSET, 4));
}

TEST(SolidSyslogSpiNorFlashOperations, FailsAnEraseWhenAnyWriteOfTheBusFails)
{
    for (int call = 1; call <= WRITES_IN_AN_ERASE; call++)
    {
        StartOver();
        Start();
        SpiNorFlashPartFake::FailWriteAt(call);
        CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED, Erase(A_BLOCK));
        CHECK_FALSE(SpiNorFlashPartFake::IsSelected());
    }
}

TEST(SolidSyslogSpiNorFlashOperations, FailsAnEraseWhenAnyReadOfTheBusFails)
{
    for (int call = 1; call <= READS_IN_AN_ERASE; call++)
    {
        StartOver();
        Start();
        SpiNorFlashPartFake::FailReadAt(call);
        CHECK_OPERATION_FAILED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_FAILED, Erase(A_BLOCK));
        CHECK_FALSE(SpiNorFlashPartFake::IsSelected());
    }
}

enum
{
    FIRST_DEVICE_BLOCK = 16,
    DEVICE_BLOCKS = 4
};

/* A SolidSyslogNorFlashBlockDevice over the flash over the part: what a store
 * is built on. The device's own behaviour is tested over NorFlashFake; this
 * shows the two fit together. */
// clang-format off
TEST_GROUP_BASE(SolidSyslogSpiNorFlashUnderABlockDevice, SpiNorFlashTestBase)
{
    struct SolidSyslogBlockDevice* device = nullptr;

    void setup() override
    {
        SetupPart();
        CreateDeviceOverANewFlash();
    }

    void teardown() override
    {
        SolidSyslogNorFlashBlockDevice_Destroy(device);
        TeardownPart();
    }

    /* The one place the device's run of blocks is given, so a restart finds
     * the run the test began with. */
    void CreateDeviceOverANewFlash()
    {
        CreateFlash();
        struct SolidSyslogNorFlashBlockDeviceConfig config = {};
        config.Flash = flash;
        config.FirstBlock = FIRST_DEVICE_BLOCK;
        config.BlockCount = DEVICE_BLOCKS;
        device = SolidSyslogNorFlashBlockDevice_Create(&config);
    }

    void Restart()
    {
        SolidSyslogNorFlashBlockDevice_Destroy(device);
        DestroyFlash();
        CreateDeviceOverANewFlash();
    }
};

// clang-format on

TEST(SolidSyslogSpiNorFlashUnderABlockDevice, GivesTheDeviceABlockLessItsState)
{
    LONGS_EQUAL(
        SECTOR_BYTES - SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_STATE_SIZE,
        SolidSyslogBlockDevice_GetBlockSize(device)
    );
}

TEST(SolidSyslogSpiNorFlashUnderABlockDevice, ErasesTheSectorOfABlockTheDeviceAcquires)
{
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 1));
    LONGS_EQUAL(1, SpiNorFlashPartFake::Erases().size());
    LONGS_EQUAL((FIRST_DEVICE_BLOCK + 1) * SECTOR_BYTES, SpiNorFlashPartFake::Erases()[0]);
    CHECK_TRUE(SolidSyslogBlockDevice_Exists(device, 1));
}

TEST(SolidSyslogSpiNorFlashUnderABlockDevice, ReadsBackWhatTheDeviceAppends)
{
    const std::vector<uint8_t> record = BytesOfPattern(1, MORE_THAN_A_PAGE);
    std::vector<uint8_t> found(record.size());
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_TRUE(SolidSyslogBlockDevice_Append(device, 0, record.data(), record.size()));
    CHECK_TRUE(SolidSyslogBlockDevice_Read(device, 0, 0, found.data(), found.size()));
    CHECK_BYTES_EQUAL(record, found);
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogSpiNorFlashUnderABlockDevice, FindsABlockAgainAfterARestart)
{
    const std::vector<uint8_t> record = {0x11, 0x22, 0x33};
    size_t size = 0;
    CHECK_TRUE(SolidSyslogBlockDevice_Acquire(device, 2));
    CHECK_TRUE(SolidSyslogBlockDevice_Append(device, 2, record.data(), record.size()));

    Restart();

    CHECK_TRUE(SolidSyslogBlockDevice_Exists(device, 2));
    CHECK_TRUE(SolidSyslogBlockDevice_Size(device, 2, &size));
    LONGS_EQUAL(record.size(), size);
}

TEST(SolidSyslogSpiNorFlashUnderABlockDevice, FailsTheDevicesAcquireWhenAnEraseOutlastsItsTime)
{
    SpiNorFlashPartFake::settings.eraseMilliseconds = FAR_LONGER_THAN_ANY_TIMEOUT;
    CHECK_FALSE(SolidSyslogBlockDevice_Acquire(device, 0));
    CHECK_IO_FAILURE_REPORTED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_TIMED_OUT);
}
