#include "BddTargetMessageSettings.h"
#include "SolidSyslog.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogHeaderFieldPrivate.h"
#include "CppUTest/TestHarness.h"

enum
{
    FORMATTER_BUFFER_SIZE = 64
};

// clang-format off
TEST_GROUP(BddTargetMessageSettings)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(FORMATTER_BUFFER_SIZE)];
    struct SolidSyslogFormatter* formatter = nullptr;
    struct SolidSyslogHeaderField field{};

    void setup() override
    {
        formatter = SolidSyslogFormatter_Create(storage, FORMATTER_BUFFER_SIZE);
        SolidSyslogHeaderField_FromFormatter(&field, formatter, FORMATTER_BUFFER_SIZE);
    }

    [[nodiscard]] const char* formatted() const
    {
        return SolidSyslogFormatter_AsFormattedBuffer(formatter);
    }
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

TEST(BddTargetMessageSettings, AnEmptyMessageIdIsRefusedAndKeepsThePrevious)
{
    BddTargetMessageSettings_SetByName("msgid", "abc");

    CHECK_FALSE(BddTargetMessageSettings_SetByName("msgid", ""));
    STRCMP_EQUAL("abc", BddTargetMessageSettings_Message()->MessageId);
}

TEST(BddTargetMessageSettings, SetMsgChangesTheMessageBody)
{
    CHECK_TRUE(BddTargetMessageSettings_SetByName("msg", "hello there"));

    STRCMP_EQUAL("hello there", BddTargetMessageSettings_Message()->Msg);
}

TEST(BddTargetMessageSettings, SetFacilityChangesTheFacility)
{
    CHECK_TRUE(BddTargetMessageSettings_SetByName("facility", "3"));

    LONGS_EQUAL(3, BddTargetMessageSettings_Message()->Facility);
}

TEST(BddTargetMessageSettings, AFacilityThatIsNotANumberIsRefusedAndKeepsThePrevious)
{
    BddTargetMessageSettings_SetByName("facility", "3");

    CHECK_FALSE(BddTargetMessageSettings_SetByName("facility", "3x"));
    LONGS_EQUAL(3, BddTargetMessageSettings_Message()->Facility);
}

TEST(BddTargetMessageSettings, AnEmptyFacilityIsRefused)
{
    CHECK_FALSE(BddTargetMessageSettings_SetByName("facility", ""));
}

TEST(BddTargetMessageSettings, SetSeverityChangesTheSeverity)
{
    CHECK_TRUE(BddTargetMessageSettings_SetByName("severity", "2"));

    LONGS_EQUAL(2, BddTargetMessageSettings_Message()->Severity);
}

TEST(BddTargetMessageSettings, ASeverityThatIsNotANumberIsRefusedAndKeepsThePrevious)
{
    BddTargetMessageSettings_SetByName("severity", "2");

    CHECK_FALSE(BddTargetMessageSettings_SetByName("severity", "x"));
    LONGS_EQUAL(2, BddTargetMessageSettings_Message()->Severity);
}

TEST(BddTargetMessageSettings, SetAppnameChangesTheAppName)
{
    CHECK_TRUE(BddTargetMessageSettings_SetByName("appname", "MyApp"));

    BddTargetMessageSettings_GetAppName(&field, nullptr);
    STRCMP_EQUAL("MyApp", formatted());
}

TEST(BddTargetMessageSettings, AnUnknownNameIsNotTakenAndChangesNothing)
{
    BddTargetMessageSettings_SetByName("msgid", "abc");

    CHECK_FALSE(BddTargetMessageSettings_SetByName("bogus", "xyz"));
    STRCMP_EQUAL("abc", BddTargetMessageSettings_Message()->MessageId);
}
