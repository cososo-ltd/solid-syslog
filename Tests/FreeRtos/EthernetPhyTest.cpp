#include <stdint.h>

#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "EthernetPhy.h"

enum
{
    REGISTER_COUNT = 32,
    CONTROL_REGISTER = 0,
    STATUS_REGISTER = 1,
    ADVERTISED_REGISTER = 4,
    PARTNER_REGISTER = 5,
    MODES_REGISTER = 18,

    CONTROL_NEGOTIATING = 0x3000,
    CONTROL_FORCED_100_FULL = 0x2100,
    CONTROL_RESET = 0x8000,

    MODE_MASK = 0x00E0,
    MODE_FORCED_100_FULL = 0x0060,
    MODE_POWERED_DOWN = 0x00C0,
    MODE_NEGOTIATE_EVERYTHING = 0x00E0,
    MODES_BESIDE_THE_MODE = 0x6001,
    NO_PHY_ON_THE_BUS = 0xFFFF,

    STATUS_LINK_UP = 0x0004,
    STATUS_NEGOTIATED = 0x0020,
    STATUS_LINK_UP_AND_NEGOTIATED = STATUS_LINK_UP | STATUS_NEGOTIATED,

    IEEE_802_3_SELECTOR = 0x0001,
    ABLE_10_HALF = 0x0020,
    ABLE_10_FULL = 0x0040,
    ABLE_100_HALF = 0x0080,
    ABLE_100_FULL = 0x0100,
    ABLE_EVERYTHING = ABLE_10_HALF | ABLE_10_FULL | ABLE_100_HALF | ABLE_100_FULL
};

static uint16_t registers[REGISTER_COUNT];
static bool statusIsStale;
static uint16_t staleStatus;
static bool readsFail;
static int readsWhileResetting;
static void* contextSeen;

static bool ReadRegister(void* context, uint8_t registerNumber, uint16_t* value)
{
    contextSeen = context;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index) -- the code under test asks for registers 0, 1, 4, 5 and 18
    *value = registers[registerNumber];
    if ((registerNumber == STATUS_REGISTER) && statusIsStale)
    {
        *value = staleStatus;
        statusIsStale = false;
    }
    if ((registerNumber == CONTROL_REGISTER) && (readsWhileResetting > 0))
    {
        *value = CONTROL_RESET;
        readsWhileResetting--;
    }
    return !readsFail;
}

static bool writesFail;
static bool writesAreLost;
static int writeCount;
static uint8_t lastRegisterWritten;
static uint16_t lastValueWritten;

static bool WriteRegister(void* context, uint8_t registerNumber, uint16_t value)
{
    contextSeen = context;
    if (!writesAreLost)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index) -- the code under test writes registers 0 and 18
        registers[registerNumber] = value;
        // A reset leaves the control register as the mode has it.
        if ((registerNumber == CONTROL_REGISTER) && (value == CONTROL_RESET))
        {
            bool negotiates = (registers[MODES_REGISTER] & MODE_MASK) == MODE_NEGOTIATE_EVERYTHING;
            registers[CONTROL_REGISTER] = negotiates ? CONTROL_NEGOTIATING : CONTROL_FORCED_100_FULL;
        }
    }
    writeCount++;
    lastRegisterWritten = registerNumber;
    lastValueWritten = value;
    return !writesFail;
}

#define CHECK_LINK(expected) LONGS_EQUAL((expected), EthernetPhy_ReadLink(ReadRegister, nullptr))

// clang-format off
TEST_GROUP(EthernetPhy)
{
    void setup() override
    {
        for (uint16_t& value : registers)
        {
            value = 0;
        }
        statusIsStale = false;
        staleStatus = 0;
        readsFail = false;
        readsWhileResetting = 0;
        contextSeen = nullptr;
        writesFail = false;
        writesAreLost = false;
        writeCount = 0;
        lastRegisterWritten = 0;
        lastValueWritten = 0;
    }

    static void GivenALinkWhosePartnerAdvertises(uint16_t partnerAbilities)
    {
        registers[STATUS_REGISTER] = STATUS_LINK_UP_AND_NEGOTIATED;
        registers[ADVERTISED_REGISTER] = ABLE_EVERYTHING;
        registers[PARTNER_REGISTER] = partnerAbilities;
    }
};
// clang-format on

TEST(EthernetPhy, LinkIsDownWhenTheStatusRegisterSaysSo)
{
    CHECK_LINK(ETHERNET_PHY_LINK_DOWN);
}

TEST(EthernetPhy, LinkIs100FullWhenBothEndsAdvertiseIt)
{
    GivenALinkWhosePartnerAdvertises(ABLE_EVERYTHING);
    CHECK_LINK(ETHERNET_PHY_LINK_100_FULL);
}

TEST(EthernetPhy, LinkIs100HalfWhenThatIsTheBestBothEndsAdvertise)
{
    GivenALinkWhosePartnerAdvertises(ABLE_100_HALF | ABLE_10_FULL | ABLE_10_HALF);
    CHECK_LINK(ETHERNET_PHY_LINK_100_HALF);
}

TEST(EthernetPhy, LinkIs10FullWhenThatIsTheBestBothEndsAdvertise)
{
    GivenALinkWhosePartnerAdvertises(ABLE_10_FULL | ABLE_10_HALF);
    CHECK_LINK(ETHERNET_PHY_LINK_10_FULL);
}

TEST(EthernetPhy, LinkIs10HalfWhenThatIsAllBothEndsAdvertise)
{
    GivenALinkWhosePartnerAdvertises(ABLE_10_HALF);
    CHECK_LINK(ETHERNET_PHY_LINK_10_HALF);
}

TEST(EthernetPhy, LinkIsDownWhenTheEndsAdvertiseNothingInCommon)
{
    GivenALinkWhosePartnerAdvertises(ABLE_10_HALF);
    registers[ADVERTISED_REGISTER] = ABLE_100_FULL;
    CHECK_LINK(ETHERNET_PHY_LINK_DOWN);
}

TEST(EthernetPhy, LinkIsDownWhileNegotiationIsStillRunning)
{
    GivenALinkWhosePartnerAdvertises(ABLE_EVERYTHING);
    registers[STATUS_REGISTER] = STATUS_LINK_UP;
    CHECK_LINK(ETHERNET_PHY_LINK_DOWN);
}

TEST(EthernetPhy, LinkIsDownWhenNegotiationFinishedButTheLinkDidNotHold)
{
    GivenALinkWhosePartnerAdvertises(ABLE_EVERYTHING);
    registers[STATUS_REGISTER] = STATUS_NEGOTIATED;
    CHECK_LINK(ETHERNET_PHY_LINK_DOWN);
}

TEST(EthernetPhy, LinkIsUpWhenTheStatusRegisterStillHoldsAnEarlierDrop)
{
    GivenALinkWhosePartnerAdvertises(ABLE_EVERYTHING);
    statusIsStale = true;
    staleStatus = STATUS_NEGOTIATED;
    CHECK_LINK(ETHERNET_PHY_LINK_100_FULL);
}

TEST(EthernetPhy, LinkIsDownWhenThePhyCannotBeRead)
{
    GivenALinkWhosePartnerAdvertises(ABLE_EVERYTHING);
    readsFail = true;
    CHECK_LINK(ETHERNET_PHY_LINK_DOWN);
}

TEST(EthernetPhy, ReadIsGivenTheCallersContext)
{
    int context = 0;
    (void) EthernetPhy_ReadLink(ReadRegister, &context);
    POINTERS_EQUAL(&context, contextSeen);
}

TEST(EthernetPhy, OnlyADownLinkIsNotUp)
{
    CHECK_FALSE(EthernetPhy_IsUp(ETHERNET_PHY_LINK_DOWN));
    CHECK_TRUE(EthernetPhy_IsUp(ETHERNET_PHY_LINK_10_HALF));
    CHECK_TRUE(EthernetPhy_IsUp(ETHERNET_PHY_LINK_100_FULL));
}

TEST(EthernetPhy, FullDuplexIsToldFromHalf)
{
    CHECK_TRUE(EthernetPhy_IsFullDuplex(ETHERNET_PHY_LINK_10_FULL));
    CHECK_TRUE(EthernetPhy_IsFullDuplex(ETHERNET_PHY_LINK_100_FULL));
    CHECK_FALSE(EthernetPhy_IsFullDuplex(ETHERNET_PHY_LINK_10_HALF));
    CHECK_FALSE(EthernetPhy_IsFullDuplex(ETHERNET_PHY_LINK_100_HALF));
}

TEST(EthernetPhy, OneHundredMegabitIsToldFromTen)
{
    CHECK_TRUE(EthernetPhy_Is100Megabit(ETHERNET_PHY_LINK_100_HALF));
    CHECK_TRUE(EthernetPhy_Is100Megabit(ETHERNET_PHY_LINK_100_FULL));
    CHECK_FALSE(EthernetPhy_Is100Megabit(ETHERNET_PHY_LINK_10_HALF));
    CHECK_FALSE(EthernetPhy_Is100Megabit(ETHERNET_PHY_LINK_10_FULL));
}

TEST(EthernetPhy, EachLinkHasItsOwnText)
{
    STRCMP_EQUAL("down", EthernetPhy_LinkText(ETHERNET_PHY_LINK_DOWN));
    STRCMP_EQUAL("up, 10 Mbit/s half duplex", EthernetPhy_LinkText(ETHERNET_PHY_LINK_10_HALF));
    STRCMP_EQUAL("up, 10 Mbit/s full duplex", EthernetPhy_LinkText(ETHERNET_PHY_LINK_10_FULL));
    STRCMP_EQUAL("up, 100 Mbit/s half duplex", EthernetPhy_LinkText(ETHERNET_PHY_LINK_100_HALF));
    STRCMP_EQUAL("up, 100 Mbit/s full duplex", EthernetPhy_LinkText(ETHERNET_PHY_LINK_100_FULL));
}

// The PHY takes its mode from strap pins as it leaves reset, and on this board
// does not always take the one it is strapped for.
TEST(EthernetPhy, RestartingPutsThePhyInTheModeThatNegotiatesEverything)
{
    registers[MODES_REGISTER] = MODE_FORCED_100_FULL;

    CHECK_TRUE(EthernetPhy_Restart(WriteRegister, ReadRegister, nullptr));

    LONGS_EQUAL(MODE_NEGOTIATE_EVERYTHING, registers[MODES_REGISTER] & MODE_MASK);
    LONGS_EQUAL(CONTROL_NEGOTIATING, registers[CONTROL_REGISTER]);
}

TEST(EthernetPhy, RestartingKeepsWhatElseTheModesRegisterHolds)
{
    registers[MODES_REGISTER] = MODES_BESIDE_THE_MODE | MODE_POWERED_DOWN;

    EthernetPhy_Restart(WriteRegister, ReadRegister, nullptr);

    LONGS_EQUAL(MODES_BESIDE_THE_MODE | MODE_NEGOTIATE_EVERYTHING, registers[MODES_REGISTER]);
}

// The mode is taken up by the reset that follows it.
TEST(EthernetPhy, RestartingResetsThePhyOnceTheModeIsSet)
{
    EthernetPhy_Restart(WriteRegister, ReadRegister, nullptr);

    LONGS_EQUAL(2, writeCount);
    LONGS_EQUAL(CONTROL_REGISTER, lastRegisterWritten);
    LONGS_EQUAL(CONTROL_RESET, lastValueWritten);
}

TEST(EthernetPhy, RestartingWaitsForTheResetToEnd)
{
    readsWhileResetting = 5;

    CHECK_TRUE(EthernetPhy_Restart(WriteRegister, ReadRegister, nullptr));

    LONGS_EQUAL(0, readsWhileResetting);
}

TEST(EthernetPhy, RestartingFailsWhenTheResetNeverEnds)
{
    readsWhileResetting = 100000;

    CHECK_FALSE(EthernetPhy_Restart(WriteRegister, ReadRegister, nullptr));
}

// A write on the management bus is not acknowledged, so only reading the
// control register back says whether the PHY took it.
TEST(EthernetPhy, RestartingFailsWhenThePhyIsStillForcedToAMode)
{
    registers[MODES_REGISTER] = MODE_FORCED_100_FULL;
    registers[CONTROL_REGISTER] = CONTROL_FORCED_100_FULL;
    writesAreLost = true;

    CHECK_FALSE(EthernetPhy_Restart(WriteRegister, ReadRegister, nullptr));
}

TEST(EthernetPhy, RestartingFailsWhenNoPhyDrivesTheBus)
{
    registers[MODES_REGISTER] = NO_PHY_ON_THE_BUS;
    registers[CONTROL_REGISTER] = NO_PHY_ON_THE_BUS;
    writesAreLost = true;

    CHECK_FALSE(EthernetPhy_Restart(WriteRegister, ReadRegister, nullptr));
}

TEST(EthernetPhy, RestartingFailsWhenAWriteDoesNotComplete)
{
    writesFail = true;

    CHECK_FALSE(EthernetPhy_Restart(WriteRegister, ReadRegister, nullptr));
}

TEST(EthernetPhy, RestartingWritesNothingWhenTheModesRegisterCannotBeRead)
{
    readsFail = true;

    CHECK_FALSE(EthernetPhy_Restart(WriteRegister, ReadRegister, nullptr));

    LONGS_EQUAL(0, writeCount);
}

TEST(EthernetPhy, RestartingHandsTheCallersContextToTheAccesses)
{
    int context = 0;

    EthernetPhy_Restart(WriteRegister, ReadRegister, &context);

    POINTERS_EQUAL(&context, contextSeen);
}
