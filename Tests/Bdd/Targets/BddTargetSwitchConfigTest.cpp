#include "BddTargetSwitchConfig.h"
#include "CppUTest/TestHarness.h"

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
    LONGS_EQUAL(BDD_TARGET_SWITCH_UDP, BddTargetSwitchConfig_Selector(nullptr));
}

TEST(BddTargetSwitchConfig, SetByNameTcpSelectsTcpIndex)
{
    BddTargetSwitchConfig_SetByName("tcp");
    LONGS_EQUAL(BDD_TARGET_SWITCH_TCP, BddTargetSwitchConfig_Selector(nullptr));
}

TEST(BddTargetSwitchConfig, SetByNameTlsSelectsTlsIndex)
{
    BddTargetSwitchConfig_SetByName("tls");
    LONGS_EQUAL(BDD_TARGET_SWITCH_TLS, BddTargetSwitchConfig_Selector(nullptr));
}

TEST(BddTargetSwitchConfig, UnknownNameLeavesPreviousSelection)
{
    BddTargetSwitchConfig_SetByName("tcp");
    BddTargetSwitchConfig_SetByName("bogus");
    LONGS_EQUAL(BDD_TARGET_SWITCH_TCP, BddTargetSwitchConfig_Selector(nullptr));
}

TEST(BddTargetSwitchConfig, EmptyNameLeavesPreviousSelection)
{
    BddTargetSwitchConfig_SetByName("tcp");
    BddTargetSwitchConfig_SetByName("");
    LONGS_EQUAL(BDD_TARGET_SWITCH_TCP, BddTargetSwitchConfig_Selector(nullptr));
}

TEST(BddTargetSwitchConfig, SwitchingBackAndForthTracksLatest)
{
    BddTargetSwitchConfig_SetByName("tcp");
    BddTargetSwitchConfig_SetByName("udp");
    LONGS_EQUAL(BDD_TARGET_SWITCH_UDP, BddTargetSwitchConfig_Selector(nullptr));
    BddTargetSwitchConfig_SetByName("tcp");
    LONGS_EQUAL(BDD_TARGET_SWITCH_TCP, BddTargetSwitchConfig_Selector(nullptr));
}
