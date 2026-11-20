#include <array>
#include <cstddef>
#include <cstdint>

#include "ErrorHandlerFake.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNorFlash.h"
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

    /* Where the fixture's SFDP keeps things. */
    SFDP_MAJOR_REVISION = 5,
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

    /* The transfers a start makes of a part that is ready: one write and one
     * read for each thing it asks. */
    TRANSFERS_IN_A_START = 5,
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

/* A macro, so a failure is reported at the test's own line. */
#define CHECK_CREATED() CHECK_TRUE(flash != SolidSyslogNullNorFlash_Get())

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

TEST(SolidSyslogSpiNorFlash, DoesNotStartWithTheLineHeldLow)
{
    SpiNorFlashPartFake::settings.lineHeldLow = true;
    CreateFlash();
    CHECK_DID_NOT_START(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_ANSWER);
}

TEST(SolidSyslogSpiNorFlash, DoesNotStartAPartThatGivesNoMaker)
{
    SpiNorFlashPartFake::settings.jedecId[0] = 0xFF;
    CreateFlash();
    CHECK_DID_NOT_START(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_NO_ANSWER);
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

TEST(SolidSyslogSpiNorFlash, RefusesAPartWithAnyProtectionBitSet)
{
    for (unsigned bit = 2; bit <= 6; bit++)
    {
        ErrorHandlerFake_Install(nullptr);
        SpiNorFlashPartFake::settings.status = static_cast<uint8_t>(1U << bit);
        CreateFlash();
        CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_PART_PROTECTED);
    }
}

TEST(SolidSyslogSpiNorFlash, TakesAPartWithOnlyTheTopBitOfItsStatusSet)
{
    SpiNorFlashPartFake::settings.status = 0x80;
    CreateFlash();
    CHECK_CREATED();
}

TEST(SolidSyslogSpiNorFlash, NeverUnlocksAPartItRefusesAsProtected)
{
    SpiNorFlashPartFake::settings.status = 0x7C;
    CreateFlash();
    LONGS_EQUAL(0, SpiNorFlashPartFake::CountOf(WRITE_ENABLE));
    LONGS_EQUAL(0, SpiNorFlashPartFake::CountOf(WRITE_STATUS));
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
    const struct SolidSyslogSpiNorFlashBus whole = *SpiNorFlashPartFake::Bus();
    std::array<struct SolidSyslogSpiNorFlashBus, 5> incomplete = {whole, whole, whole, whole, whole};
    incomplete[0].Select = nullptr;
    incomplete[1].Deselect = nullptr;
    incomplete[2].Write = nullptr;
    incomplete[3].Read = nullptr;
    incomplete[4].Milliseconds = nullptr;

    for (auto& bus : incomplete)
    {
        ErrorHandlerFake_Install(nullptr);
        const struct SolidSyslogSpiNorFlashConfig config = {&bus};
        flash = SolidSyslogSpiNorFlash_Create(&config);
        CHECK_REFUSED(SOLIDSYSLOG_SPI_NOR_FLASH_ERROR_BUS_INCOMPLETE);
    }
}

TEST(SolidSyslogSpiNorFlash, AsksNothingOfABusItRefuses)
{
    const struct SolidSyslogSpiNorFlashConfig config = {nullptr};
    flash = SolidSyslogSpiNorFlash_Create(&config);
    LONGS_EQUAL(0, SpiNorFlashPartFake::Commands().size());
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
    SpiNorFlashPartFake::settings.answersSfdp = true;
    FillPool();
    CHECK_TRUE(pooled[0] != SolidSyslogNullNorFlash_Get());
}

TEST(SolidSyslogSpiNorFlashPool, HasASlotAgainOnceAFlashIsDestroyed)
{
    FillPool();
    SolidSyslogSpiNorFlash_Destroy(pooled[0]);
    pooled[0] = nullptr;
    CreateFlash();
    CHECK_CREATED();
}
