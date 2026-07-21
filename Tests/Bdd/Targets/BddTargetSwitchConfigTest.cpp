#include "BddTargetSwitchConfig.h"
#include "CppUTest/TestHarness.h"

#define CHECK_SELECTED_INDEX(expected) LONGS_EQUAL(expected, BddTargetSwitchConfig_Selector(nullptr))

// clang-format off
TEST_GROUP(BddTargetSwitchConfig)
{
    void setup() override
    {
        BddTargetSwitchConfig_SetByName("udp");
    }
};

// clang-format on

TEST(BddTargetSwitchConfig, SetByNameUdpSelectsUdpIndex)
{
    BddTargetSwitchConfig_SetByName("udp");
    CHECK_SELECTED_INDEX(BDD_TARGET_SWITCH_UDP);
}

TEST(BddTargetSwitchConfig, SetByNameTcpSelectsTcpIndex)
{
    BddTargetSwitchConfig_SetByName("tcp");
    CHECK_SELECTED_INDEX(BDD_TARGET_SWITCH_TCP);
}

TEST(BddTargetSwitchConfig, SetByNameTlsSelectsTlsIndex)
{
    BddTargetSwitchConfig_SetByName("tls");
    CHECK_SELECTED_INDEX(BDD_TARGET_SWITCH_TLS);
}

TEST(BddTargetSwitchConfig, UnknownNameLeavesPreviousSelection)
{
    BddTargetSwitchConfig_SetByName("tcp");
    BddTargetSwitchConfig_SetByName("bogus");
    CHECK_SELECTED_INDEX(BDD_TARGET_SWITCH_TCP);
}

TEST(BddTargetSwitchConfig, EmptyNameLeavesPreviousSelection)
{
    BddTargetSwitchConfig_SetByName("tcp");
    BddTargetSwitchConfig_SetByName("");
    CHECK_SELECTED_INDEX(BDD_TARGET_SWITCH_TCP);
}

TEST(BddTargetSwitchConfig, SwitchingBackAndForthTracksLatest)
{
    BddTargetSwitchConfig_SetByName("tcp");
    BddTargetSwitchConfig_SetByName("udp");
    CHECK_SELECTED_INDEX(BDD_TARGET_SWITCH_UDP);
    BddTargetSwitchConfig_SetByName("tcp");
    CHECK_SELECTED_INDEX(BDD_TARGET_SWITCH_TCP);
}
