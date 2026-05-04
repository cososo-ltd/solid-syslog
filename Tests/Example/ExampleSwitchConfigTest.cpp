#include "ExampleSwitchConfig.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(ExampleSwitchConfig)
{
    void setup() override
    {
        ExampleSwitchConfig_SetByName("udp");
    }
};

// clang-format on

TEST(ExampleSwitchConfig, SetByNameUdpSelectsUdpIndex)
{
    ExampleSwitchConfig_SetByName("udp");
    LONGS_EQUAL(EXAMPLE_SWITCH_UDP, ExampleSwitchConfig_Selector());
}

TEST(ExampleSwitchConfig, SetByNameTcpSelectsTcpIndex)
{
    ExampleSwitchConfig_SetByName("tcp");
    LONGS_EQUAL(EXAMPLE_SWITCH_TCP, ExampleSwitchConfig_Selector());
}

TEST(ExampleSwitchConfig, SetByNameTlsSelectsTlsIndex)
{
    ExampleSwitchConfig_SetByName("tls");
    LONGS_EQUAL(EXAMPLE_SWITCH_TLS, ExampleSwitchConfig_Selector());
}

TEST(ExampleSwitchConfig, UnknownNameLeavesPreviousSelection)
{
    ExampleSwitchConfig_SetByName("tcp");
    ExampleSwitchConfig_SetByName("bogus");
    LONGS_EQUAL(EXAMPLE_SWITCH_TCP, ExampleSwitchConfig_Selector());
}

TEST(ExampleSwitchConfig, EmptyNameLeavesPreviousSelection)
{
    ExampleSwitchConfig_SetByName("tcp");
    ExampleSwitchConfig_SetByName("");
    LONGS_EQUAL(EXAMPLE_SWITCH_TCP, ExampleSwitchConfig_Selector());
}

TEST(ExampleSwitchConfig, SwitchingBackAndForthTracksLatest)
{
    ExampleSwitchConfig_SetByName("tcp");
    ExampleSwitchConfig_SetByName("udp");
    LONGS_EQUAL(EXAMPLE_SWITCH_UDP, ExampleSwitchConfig_Selector());
    ExampleSwitchConfig_SetByName("tcp");
    LONGS_EQUAL(EXAMPLE_SWITCH_TCP, ExampleSwitchConfig_Selector());
}
