#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>

#include "SolidSyslogExample.h"
#include "ClockFake.h"
#include "SocketFake.h"
#include "CppUTest/TestHarness.h"

static const char* const STDIN_SEND_ONE   = "/tmp/solidsyslog_test_send1.txt";
static const char* const STDIN_SEND_THREE = "/tmp/solidsyslog_test_send3.txt";

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- path and content are semantically distinct
static void CreateInputFile(const char* path, const char* content)
{
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- fopen/fclose is POSIX C; no owning memory concern
    FILE* f = std::fopen(path, "w");
    // NOLINTNEXTLINE(clang-analyzer-unix.Stream) -- test helper; fopen failure is a test infrastructure error
    std::fputs(content, f);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- fclose is POSIX C; no owning memory concern
    std::fclose(f);
}

static void RedirectStdin(const char* path)
{
    // cppcheck-suppress ignoredReturnValue -- test helper; freopen failure is a test infrastructure error
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- freopen redirects stdin for test; no ownership transfer
    std::freopen(path, "r", stdin);
}

// clang-format off
TEST_GROUP(SolidSyslogExample)
{
    void setup() override
    {
        SocketFake_Reset();
        ClockFake_Reset();
        ClockFake_SetTime(1743768600, 0);
        optind = 1;
        CreateInputFile(STDIN_SEND_ONE, "send\nquit\n");
        CreateInputFile(STDIN_SEND_THREE, "send 3\nquit\n");
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static) -- CppUTest TEST_GROUP method
    int Run(int argc, char* argv[])
    {
        return SolidSyslogExample_Run(argc, argv);
    }

    int RunWithNoArgs()
    {
        RedirectStdin(STDIN_SEND_ONE);
        char  arg0[] = "SolidSyslogExample";
        char* argv[] = {arg0, nullptr};
        return Run(1, argv);
    }
};

// clang-format on

TEST(SolidSyslogExample, InvalidOptionReturnsOne)
{
    char  arg0[] = "SolidSyslogExample";
    char  arg1[] = "--invalid";
    char* argv[] = {arg0, arg1, nullptr};
    LONGS_EQUAL(1, Run(2, argv));
}

TEST(SolidSyslogExample, InvalidStoreReturnsOne)
{
    char  arg0[] = "SolidSyslogExample";
    char  arg1[] = "--store";
    char  arg2[] = "invalid";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(1, Run(3, argv));
}

TEST(SolidSyslogExample, RunWithNoArgsReturnsZero)
{
    LONGS_EQUAL(0, RunWithNoArgs());
}

TEST(SolidSyslogExample, RunSendsOneMessage)
{
    RunWithNoArgs();
    LONGS_EQUAL(1, SocketFake_SendtoCallCount());
}

TEST(SolidSyslogExample, DefaultMessageContainsLocal0InfoPrival)
{
    RunWithNoArgs();
    STRNCMP_EQUAL("<134>", SocketFake_LastBufAsString(), 5);
}

TEST(SolidSyslogExample, FacilityFlagAppearsInPrival)
{
    RedirectStdin(STDIN_SEND_ONE);
    char  arg0[] = "SolidSyslogExample";
    char  arg1[] = "--facility";
    char  arg2[] = "0";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Run(3, argv);
    STRNCMP_EQUAL("<6>", SocketFake_LastBufAsString(), 3);
}

TEST(SolidSyslogExample, SeverityFlagAppearsInPrival)
{
    RedirectStdin(STDIN_SEND_ONE);
    char  arg0[] = "SolidSyslogExample";
    char  arg1[] = "--severity";
    char  arg2[] = "0";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Run(3, argv);
    STRNCMP_EQUAL("<128>", SocketFake_LastBufAsString(), 5);
}

TEST(SolidSyslogExample, ResolvesConfiguredHost)
{
    RunWithNoArgs();
    STRCMP_EQUAL("syslog-ng", SocketFake_LastGetAddrInfoHostname());
}

TEST(SolidSyslogExample, SendsToConfiguredPort)
{
    RunWithNoArgs();
    LONGS_EQUAL(5514, SocketFake_LastPort());
}

TEST(SolidSyslogExample, AppNameDerivedFromArgv0)
{
    RedirectStdin(STDIN_SEND_ONE);
    char  arg0[] = "/usr/bin/MyApp";
    char* argv[] = {arg0, nullptr};
    Run(1, argv);
    const char* msg = SocketFake_LastBufAsString();
    CHECK(strstr(msg, "MyApp") != nullptr);
}

TEST(SolidSyslogExample, SocketCreatedWithUdpDgram)
{
    RunWithNoArgs();
    LONGS_EQUAL(AF_INET, SocketFake_SocketDomain());
    LONGS_EQUAL(SOCK_DGRAM, SocketFake_SocketType());
}

TEST(SolidSyslogExample, SocketClosedAfterRun)
{
    RunWithNoArgs();
    LONGS_EQUAL(1, SocketFake_CloseCallCount());
}

TEST(SolidSyslogExample, MsgIdFlagAppearsInMessage)
{
    RedirectStdin(STDIN_SEND_ONE);
    char  arg0[] = "SolidSyslogExample";
    char  arg1[] = "--msgid";
    char  arg2[] = "ID47";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Run(3, argv);
    CHECK(std::strstr(SocketFake_LastBufAsString(), "ID47") != nullptr);
}

TEST(SolidSyslogExample, SendCommandSendsMultipleMessages)
{
    RedirectStdin(STDIN_SEND_THREE);
    char  arg0[] = "SolidSyslogExample";
    char* argv[] = {arg0, nullptr};
    Run(1, argv);
    LONGS_EQUAL(3, SocketFake_SendtoCallCount());
}

TEST(SolidSyslogExample, MessageFlagAppearsInMessage)
{
    RedirectStdin(STDIN_SEND_ONE);
    char  arg0[] = "SolidSyslogExample";
    char  arg1[] = "--message";
    char  arg2[] = "system started";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    Run(3, argv);
    CHECK(std::strstr(SocketFake_LastBufAsString(), "system started") != nullptr);
}
