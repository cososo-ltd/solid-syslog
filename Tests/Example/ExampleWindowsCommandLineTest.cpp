#include "CppUTest/TestHarness.h"
#include "ExampleWindowsCommandLine.h"

// clang-format off
TEST_GROUP(ExampleWindowsCommandLine)
{
    struct WindowsExampleOptions options = {};

    void Parse(int argc, char* argv[])
    {
        ExampleWindowsCommandLine_Parse(argc, argv, &options);
    }
};

// clang-format on

TEST(ExampleWindowsCommandLine, DefaultFacilityIsLocal0)
{
    char  arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    LONGS_EQUAL(SOLIDSYSLOG_FACILITY_LOCAL0, options.facility);
}

TEST(ExampleWindowsCommandLine, DefaultSeverityIsInfo)
{
    char  arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_INFO, options.severity);
}

TEST(ExampleWindowsCommandLine, DefaultMessageIdIsNull)
{
    char  arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    POINTERS_EQUAL(nullptr, options.messageId);
}

TEST(ExampleWindowsCommandLine, DefaultMsgIsNull)
{
    char  arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    POINTERS_EQUAL(nullptr, options.msg);
}

TEST(ExampleWindowsCommandLine, FacilityFlagSetsFacility)
{
    char  arg0[] = "test";
    char  arg1[] = "--facility";
    char  arg2[] = "23";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Parse(3, argv);
    LONGS_EQUAL(23, options.facility);
}

TEST(ExampleWindowsCommandLine, SeverityFlagSetsSeverity)
{
    char  arg0[] = "test";
    char  arg1[] = "--severity";
    char  arg2[] = "7";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Parse(3, argv);
    LONGS_EQUAL(7, options.severity);
}

TEST(ExampleWindowsCommandLine, MsgidFlagSetsMessageId)
{
    char  arg0[] = "test";
    char  arg1[] = "--msgid";
    char  arg2[] = "ID47";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Parse(3, argv);
    STRCMP_EQUAL("ID47", options.messageId);
}

TEST(ExampleWindowsCommandLine, MessageFlagSetsMsg)
{
    char  arg0[] = "test";
    char  arg1[] = "--message";
    char  arg2[] = "system started";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Parse(3, argv);
    STRCMP_EQUAL("system started", options.msg);
}

TEST(ExampleWindowsCommandLine, DefaultTransportIsUdp)
{
    char  arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    STRCMP_EQUAL("udp", options.transport);
}

TEST(ExampleWindowsCommandLine, TransportFlagSetsTcp)
{
    char  arg0[] = "test";
    char  arg1[] = "--transport";
    char  arg2[] = "tcp";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Parse(3, argv);
    STRCMP_EQUAL("tcp", options.transport);
}

TEST(ExampleWindowsCommandLine, AllFlagsTogether)
{
    char  arg0[] = "test";
    char  arg1[] = "--facility";
    char  arg2[] = "16";
    char  arg3[] = "--severity";
    char  arg4[] = "6";
    char  arg5[] = "--msgid";
    char  arg6[] = "CONN";
    char  arg7[] = "--message";
    char  arg8[] = "session opened";
    char* argv[] = {arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, nullptr};
    Parse(9, argv);
    LONGS_EQUAL(16, options.facility);
    LONGS_EQUAL(6, options.severity);
    STRCMP_EQUAL("CONN", options.messageId);
    STRCMP_EQUAL("session opened", options.msg);
}

TEST(ExampleWindowsCommandLine, UnknownFlagIsIgnored)
{
    char  arg0[] = "test";
    char  arg1[] = "--unknown";
    char  arg2[] = "value";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Parse(3, argv);
    LONGS_EQUAL(SOLIDSYSLOG_FACILITY_LOCAL0, options.facility);
    POINTERS_EQUAL(nullptr, options.messageId);
}

TEST(ExampleWindowsCommandLine, FacilityFlagWithoutValueIsIgnored)
{
    char  arg0[] = "test";
    char  arg1[] = "--facility";
    char* argv[] = {arg0, arg1, nullptr};
    Parse(2, argv);
    LONGS_EQUAL(SOLIDSYSLOG_FACILITY_LOCAL0, options.facility);
}

TEST(ExampleWindowsCommandLine, DefaultAppNameIsNull)
{
    char  arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    POINTERS_EQUAL(nullptr, options.appName);
}

TEST(ExampleWindowsCommandLine, AppNameFlagSetsAppName)
{
    char  arg0[] = "test";
    char  arg1[] = "--app-name";
    char  arg2[] = "SolidSyslogThreadedExample";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Parse(3, argv);
    STRCMP_EQUAL("SolidSyslogThreadedExample", options.appName);
}

TEST(ExampleWindowsCommandLine, DefaultHaltExitIsFalse)
{
    char  arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    CHECK_FALSE(options.haltExit);
}

TEST(ExampleWindowsCommandLine, HaltExitFlagSetsHaltExit)
{
    char  arg0[] = "test";
    char  arg1[] = "--halt-exit";
    char* argv[] = {arg0, arg1, nullptr};
    Parse(2, argv);
    CHECK_TRUE(options.haltExit);
}
