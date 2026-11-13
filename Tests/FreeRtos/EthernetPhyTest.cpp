#include <stdint.h>

#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "EthernetPhy.h"

enum
{
    REGISTER_COUNT = 32,
    STATUS_REGISTER = 1,
    ADVERTISED_REGISTER = 4,
    PARTNER_REGISTER = 5,

    STATUS_LINK_UP = 0x0004,
    STATUS_NEGOTIATED = 0x0020,
    STATUS_LINK_UP_AND_NEGOTIATED = STATUS_LINK_UP | STATUS_NEGOTIATED,

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
static void* contextSeen;

static bool ReadRegister(void* context, uint8_t registerNumber, uint16_t* value)
{
    contextSeen = context;
    *value = registers[registerNumber];
    if ((registerNumber == STATUS_REGISTER) && statusIsStale)
    {
        *value = staleStatus;
        statusIsStale = false;
    }
    return !readsFail;
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
        contextSeen = nullptr;
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
