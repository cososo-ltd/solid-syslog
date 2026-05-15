#include "CppUTest/TestHarness.h"
#include "BddTargetWindowsCommandLine.h"

// clang-format off
TEST_GROUP(BddTargetWindowsCommandLine)
{
    struct BddTargetWindowsOptions options = {};

    void Parse(int argc, char* argv[])
    {
        BddTargetWindowsCommandLine_Parse(argc, argv, &options);
    }
};

// clang-format on

TEST(BddTargetWindowsCommandLine, DefaultFacilityIsLocal0)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    LONGS_EQUAL(SolidSyslogFacility_Local0, options.Facility);
}

TEST(BddTargetWindowsCommandLine, DefaultSeverityIsInfo)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    LONGS_EQUAL(SolidSyslogSeverity_Informational, options.Severity);
}

TEST(BddTargetWindowsCommandLine, DefaultMessageIdIsNull)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    POINTERS_EQUAL(nullptr, options.MessageId);
}

TEST(BddTargetWindowsCommandLine, DefaultMsgIsNull)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    POINTERS_EQUAL(nullptr, options.Msg);
}

TEST(BddTargetWindowsCommandLine, FacilityFlagSetsFacility)
{
    char arg0[] = "test";
    char arg1[] = "--facility";
    char arg2[] = "23";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Parse(3, argv);
    LONGS_EQUAL(23, options.Facility);
}

TEST(BddTargetWindowsCommandLine, SeverityFlagSetsSeverity)
{
    char arg0[] = "test";
    char arg1[] = "--severity";
    char arg2[] = "7";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Parse(3, argv);
    LONGS_EQUAL(7, options.Severity);
}

TEST(BddTargetWindowsCommandLine, MsgidFlagSetsMessageId)
{
    char arg0[] = "test";
    char arg1[] = "--msgid";
    char arg2[] = "ID47";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Parse(3, argv);
    STRCMP_EQUAL("ID47", options.MessageId);
}

TEST(BddTargetWindowsCommandLine, MessageFlagSetsMsg)
{
    char arg0[] = "test";
    char arg1[] = "--message";
    char arg2[] = "system started";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Parse(3, argv);
    STRCMP_EQUAL("system started", options.Msg);
}

TEST(BddTargetWindowsCommandLine, DefaultTransportIsUdp)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    STRCMP_EQUAL("udp", options.Transport);
}

TEST(BddTargetWindowsCommandLine, TransportFlagSetsTcp)
{
    char arg0[] = "test";
    char arg1[] = "--transport";
    char arg2[] = "tcp";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Parse(3, argv);
    STRCMP_EQUAL("tcp", options.Transport);
}

TEST(BddTargetWindowsCommandLine, AllFlagsTogether)
{
    char arg0[] = "test";
    char arg1[] = "--facility";
    char arg2[] = "16";
    char arg3[] = "--severity";
    char arg4[] = "6";
    char arg5[] = "--msgid";
    char arg6[] = "CONN";
    char arg7[] = "--message";
    char arg8[] = "session opened";
    char* argv[] = {arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, nullptr};
    Parse(9, argv);
    LONGS_EQUAL(16, options.Facility);
    LONGS_EQUAL(6, options.Severity);
    STRCMP_EQUAL("CONN", options.MessageId);
    STRCMP_EQUAL("session opened", options.Msg);
}

TEST(BddTargetWindowsCommandLine, UnknownFlagIsIgnored)
{
    char arg0[] = "test";
    char arg1[] = "--unknown";
    char arg2[] = "value";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Parse(3, argv);
    LONGS_EQUAL(SolidSyslogFacility_Local0, options.Facility);
    POINTERS_EQUAL(nullptr, options.MessageId);
}

TEST(BddTargetWindowsCommandLine, FacilityFlagWithoutValueIsIgnored)
{
    char arg0[] = "test";
    char arg1[] = "--facility";
    char* argv[] = {arg0, arg1, nullptr};
    Parse(2, argv);
    LONGS_EQUAL(SolidSyslogFacility_Local0, options.Facility);
}

TEST(BddTargetWindowsCommandLine, DefaultAppNameIsNull)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    POINTERS_EQUAL(nullptr, options.AppName);
}

TEST(BddTargetWindowsCommandLine, AppNameFlagSetsAppName)
{
    char arg0[] = "test";
    char arg1[] = "--app-name";
    char arg2[] = "SolidSyslogThreadedExample";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Parse(3, argv);
    STRCMP_EQUAL("SolidSyslogThreadedExample", options.AppName);
}

TEST(BddTargetWindowsCommandLine, DefaultHaltExitIsFalse)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    CHECK_FALSE(options.HaltExit);
}

TEST(BddTargetWindowsCommandLine, HaltExitFlagSetsHaltExit)
{
    char arg0[] = "test";
    char arg1[] = "--halt-exit";
    char* argv[] = {arg0, arg1, nullptr};
    Parse(2, argv);
    CHECK_TRUE(options.HaltExit);
}
