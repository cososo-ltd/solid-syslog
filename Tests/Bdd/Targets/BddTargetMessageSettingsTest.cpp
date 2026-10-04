#include "BddTargetMessageSettings.h"
#include "SolidSyslog.h"
#include "SolidSyslogEndpoint.h"
#include "SolidSyslogEndpointHostPrivate.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogHeaderFieldPrivate.h"
#include "CppUTest/TestHarness.h"

enum
{
    FORMATTER_BUFFER_SIZE = 64
};

static const char TEST_DEFAULT_HOST[] = "10.0.2.2";

// clang-format off
TEST_GROUP(BddTargetMessageSettings)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(FORMATTER_BUFFER_SIZE)];
    struct SolidSyslogFormatter* formatter = nullptr;
    struct SolidSyslogHeaderField field{};

    struct SolidSyslogEndpointHost hostSink{};
    struct SolidSyslogEndpoint endpoint{};

    void setup() override
    {
        BddTargetMessageSettings_Reset(TEST_DEFAULT_HOST);
        formatter = SolidSyslogFormatter_Create(storage, FORMATTER_BUFFER_SIZE);
        SolidSyslogHeaderField_FromFormatter(&field, formatter, FORMATTER_BUFFER_SIZE);
        SolidSyslogEndpointHost_FromFormatter(&hostSink, formatter);
        endpoint.Host = &hostSink;
    }

    // The host lands in formatted(); the port in endpoint.Port.
    void ReadEndpoint()
    {
        BddTargetMessageSettings_GetEndpoint(&endpoint, nullptr);
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

TEST(BddTargetMessageSettings, AFacilityWithASignIsRefused)
{
    CHECK_FALSE(BddTargetMessageSettings_SetByName("facility", "+3"));
}

TEST(BddTargetMessageSettings, AFacilityTooLargeToParseIsRefused)
{
    CHECK_FALSE(BddTargetMessageSettings_SetByName("facility", "99999999999999999999999"));
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

TEST(BddTargetMessageSettings, SetHostChangesTheEndpointHost)
{
    CHECK_TRUE(BddTargetMessageSettings_SetByName("host", "10.1.2.3"));

    ReadEndpoint();

    STRCMP_EQUAL("10.1.2.3", formatted());
}

TEST(BddTargetMessageSettings, SetPortChangesTheEndpointPort)
{
    CHECK_TRUE(BddTargetMessageSettings_SetByName("port", "6000"));

    ReadEndpoint();

    UNSIGNED_LONGS_EQUAL(6000, endpoint.Port);
}

TEST(BddTargetMessageSettings, PortZeroIsRefused)
{
    CHECK_FALSE(BddTargetMessageSettings_SetByName("port", "0"));
}

TEST(BddTargetMessageSettings, APortAbove65535IsRefused)
{
    CHECK_FALSE(BddTargetMessageSettings_SetByName("port", "65536"));
}

TEST(BddTargetMessageSettings, Port65535IsTaken)
{
    CHECK_TRUE(BddTargetMessageSettings_SetByName("port", "65535"));
}

TEST(BddTargetMessageSettings, SettingThePortMovesTheEndpointVersion)
{
    uint32_t before = BddTargetMessageSettings_GetEndpointVersion(nullptr);

    BddTargetMessageSettings_SetByName("port", "6001");

    CHECK(BddTargetMessageSettings_GetEndpointVersion(nullptr) != before);
}

TEST(BddTargetMessageSettings, SettingTheHostMovesTheEndpointVersion)
{
    uint32_t before = BddTargetMessageSettings_GetEndpointVersion(nullptr);

    BddTargetMessageSettings_SetByName("host", "10.9.8.7");

    CHECK(BddTargetMessageSettings_GetEndpointVersion(nullptr) != before);
}

TEST(BddTargetMessageSettings, ARefusedPortLeavesTheEndpointVersionAlone)
{
    uint32_t before = BddTargetMessageSettings_GetEndpointVersion(nullptr);

    BddTargetMessageSettings_SetByName("port", "0");

    UNSIGNED_LONGS_EQUAL(before, BddTargetMessageSettings_GetEndpointVersion(nullptr));
}

TEST(BddTargetMessageSettings, ResetRestoresTheMessageDefaults)
{
    BddTargetMessageSettings_SetByName("msgid", "abc");
    BddTargetMessageSettings_SetByName("msg", "changed");
    BddTargetMessageSettings_SetByName("facility", "3");
    BddTargetMessageSettings_SetByName("severity", "2");

    BddTargetMessageSettings_Reset(TEST_DEFAULT_HOST);

    const struct SolidSyslogMessage* message = BddTargetMessageSettings_Message();
    STRCMP_EQUAL("example", message->MessageId);
    STRCMP_EQUAL("Hello from SolidSyslog", message->Msg);
    LONGS_EQUAL(SOLIDSYSLOG_FACILITY_LOCAL0, message->Facility);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_INFORMATIONAL, message->Severity);
}

TEST(BddTargetMessageSettings, ResetRestoresTheAppNameDefault)
{
    BddTargetMessageSettings_SetByName("appname", "MyApp");

    BddTargetMessageSettings_Reset(TEST_DEFAULT_HOST);

    BddTargetMessageSettings_GetAppName(&field, nullptr);
    STRCMP_EQUAL("SolidSyslogBddTarget", formatted());
}

TEST(BddTargetMessageSettings, ResetPointsTheEndpointAtTheDefaultHostAndPort)
{
    BddTargetMessageSettings_SetByName("host", "10.1.2.3");
    BddTargetMessageSettings_SetByName("port", "6000");

    BddTargetMessageSettings_Reset(TEST_DEFAULT_HOST);

    ReadEndpoint();
    STRCMP_EQUAL(TEST_DEFAULT_HOST, formatted());
    UNSIGNED_LONGS_EQUAL(5514, endpoint.Port);
}

TEST(BddTargetMessageSettings, ResetMovesTheEndpointVersion)
{
    uint32_t before = BddTargetMessageSettings_GetEndpointVersion(nullptr);

    BddTargetMessageSettings_Reset(TEST_DEFAULT_HOST);

    CHECK(BddTargetMessageSettings_GetEndpointVersion(nullptr) != before);
}

TEST(BddTargetMessageSettings, AnUnknownNameIsNotTakenAndChangesNothing)
{
    BddTargetMessageSettings_SetByName("msgid", "abc");

    CHECK_FALSE(BddTargetMessageSettings_SetByName("bogus", "xyz"));
    STRCMP_EQUAL("abc", BddTargetMessageSettings_Message()->MessageId);
}
