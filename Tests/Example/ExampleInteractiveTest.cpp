#include "ExampleInteractive.h"
#include "SolidSyslog.h"
#include "CppUTest/TestHarness.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <string>

namespace
{
struct SetHandlerSpy
{
    int         callCount;
    std::string lastName;
    std::string lastValue;
    bool        returnValue;
};

SetHandlerSpy spy;

bool RecordSet(const char* name, const char* value)
{
    spy.callCount++;
    spy.lastName  = name;
    spy.lastValue = value;
    return spy.returnValue;
}

const char* const STDOUT_CAPTURE_PATH = "/tmp/solidsyslog_example_interactive_stdout.txt";
int               saved_stdout_fd     = -1;

void StartStdoutCapture()
{
    fflush(stdout);
    saved_stdout_fd = dup(fileno(stdout));
    (void) freopen(STDOUT_CAPTURE_PATH, "w", stdout);
    setvbuf(stdout, nullptr, _IONBF, 0);
}

std::string EndStdoutCapture()
{
    fflush(stdout);
    (void) dup2(saved_stdout_fd, fileno(stdout));
    close(saved_stdout_fd);
    saved_stdout_fd = -1;

    std::string content;
    FILE*       f = fopen(STDOUT_CAPTURE_PATH, "r");
    if (f != nullptr)
    {
        char   buf[2048];
        size_t n = 0;
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        {
            content.append(buf, n);
        }
        fclose(f);
    }
    return content;
}

void RunWithInput(const char* input, ExampleInteractiveSetHandler onSet)
{
    FILE*                     in      = fmemopen((void*) input, strlen(input), "r");
    struct SolidSyslogMessage message = {};
    ExampleInteractive_Run(&message, in, nullptr, onSet);
    fclose(in);
}

std::string RunCapturingStdout(const char* input, ExampleInteractiveSetHandler onSet)
{
    StartStdoutCapture();
    RunWithInput(input, onSet);
    return EndStdoutCapture();
}
} // namespace

// clang-format off
TEST_GROUP(ExampleInteractive)
{
    void setup() override
    {
        spy             = SetHandlerSpy{};
        spy.returnValue = true;
    }
};
// clang-format on

TEST(ExampleInteractive, SetHandlerNotCalledWithQuitOnly)
{
    RunWithInput("quit\n", RecordSet);

    LONGS_EQUAL(0, spy.callCount);
}

TEST(ExampleInteractive, SetCommandCallsHandlerOnce)
{
    RunWithInput("set hostname Foo\nquit\n", RecordSet);

    LONGS_EQUAL(1, spy.callCount);
}

TEST(ExampleInteractive, SetCommandPassesNameToHandler)
{
    RunWithInput("set hostname Foo\nquit\n", RecordSet);

    STRCMP_EQUAL("hostname", spy.lastName.c_str());
}

TEST(ExampleInteractive, SetCommandPassesValueToHandler)
{
    RunWithInput("set hostname Foo\nquit\n", RecordSet);

    STRCMP_EQUAL("Foo", spy.lastValue.c_str());
}

TEST(ExampleInteractive, SetCommandWithoutValueGivesEmptyValue)
{
    RunWithInput("set hostname\nquit\n", RecordSet);

    STRCMP_EQUAL("hostname", spy.lastName.c_str());
    STRCMP_EQUAL("", spy.lastValue.c_str());
}

TEST(ExampleInteractive, SetCommandWithEmbeddedSpacesPreservesValueAfterFirst)
{
    RunWithInput("set msg some text\nquit\n", RecordSet);

    STRCMP_EQUAL("msg", spy.lastName.c_str());
    STRCMP_EQUAL("some text", spy.lastValue.c_str());
}

TEST(ExampleInteractive, NullSetHandlerSilentlyIgnoresSetLine)
{
    RunWithInput("set hostname Foo\nquit\n", nullptr);

    LONGS_EQUAL(0, spy.callCount);
}

TEST(ExampleInteractive, SetCommandPrintsEchoOnHandlerSuccess)
{
    spy.returnValue = true;

    std::string captured = RunCapturingStdout("set hostname Foo\nquit\n", RecordSet);

    CHECK(captured.find("set hostname=Foo") != std::string::npos);
}

TEST(ExampleInteractive, SetCommandPrintsInvalidOnHandlerFailure)
{
    spy.returnValue = false;

    std::string captured = RunCapturingStdout("set hostname Foo\nquit\n", RecordSet);

    CHECK(captured.find("set: invalid") != std::string::npos);
}

/* Test list (ZOMBIES order):
 *  Z  [x] SetHandlerNotCalledWithQuitOnly
 *  O  [x] SetCommandCallsHandlerOnce
 *  O.2[x] SetCommandPassesNameToHandler
 *  O.3[x] SetCommandPassesValueToHandler
 *  B  [x] SetCommandWithoutValueGivesEmptyValue (locked in)
 *  B  [x] SetCommandWithEmbeddedSpacesPreservesValueAfterFirst (locked in)
 *  E  [x] NullSetHandlerSilentlyIgnoresSetLine (locked in)
 *  I  [x] SetCommandPrintsEchoOnHandlerSuccess
 *  I  [x] SetCommandPrintsInvalidOnHandlerFailure
 */
