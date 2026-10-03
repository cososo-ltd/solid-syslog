#include "BddTargetMessageSettings.h"
#include "CppUTest/TestHarness.h"

#include "SolidSyslog.h"

// clang-format off
TEST_GROUP(BddTargetMessageSettings)
{
};

// clang-format on

TEST(BddTargetMessageSettings, SetMsgidChangesTheMessageId)
{
    CHECK_TRUE(BddTargetMessageSettings_SetByName("msgid", "abc"));

    STRCMP_EQUAL("abc", BddTargetMessageSettings_Message()->MessageId);
}

TEST(BddTargetMessageSettings, AMessageIdLongerThan32CharactersIsRefusedAndKeepsThePrevious)
{
    BddTargetMessageSettings_SetByName("msgid", "abc");

    CHECK_FALSE(BddTargetMessageSettings_SetByName("msgid", "123456789012345678901234567890123"));
    STRCMP_EQUAL("abc", BddTargetMessageSettings_Message()->MessageId);
}

TEST(BddTargetMessageSettings, AMessageIdOfExactly32CharactersIsTaken)
{
    CHECK_TRUE(BddTargetMessageSettings_SetByName("msgid", "12345678901234567890123456789012"));
    STRCMP_EQUAL("12345678901234567890123456789012", BddTargetMessageSettings_Message()->MessageId);
}

TEST(BddTargetMessageSettings, AnUnknownNameIsNotTakenAndChangesNothing)
{
    BddTargetMessageSettings_SetByName("msgid", "abc");

    CHECK_FALSE(BddTargetMessageSettings_SetByName("bogus", "xyz"));
    STRCMP_EQUAL("abc", BddTargetMessageSettings_Message()->MessageId);
}
