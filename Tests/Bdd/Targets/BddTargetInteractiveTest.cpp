#include "BddTargetInteractive.h"
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
    int callCount = 0;
    std::string lastName;
    std::string lastValue;
    bool returnValue = false;
};

SetHandlerSpy spy;

bool RecordSet(const char* name, const char* value)
{
    spy.callCount++;
    spy.lastName = name;
    spy.lastValue = value;
    return spy.returnValue;
}

const char* const STDOUT_CAPTURE_PATH = "/tmp/solidsyslog_example_interactive_stdout.txt";
int saved_stdout_fd = -1;

void StartStdoutCapture()
{
    fflush(stdout);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- test helper; FD is hand-managed and restored in EndStdoutCapture
    saved_stdout_fd = dup(fileno(stdout));
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- freopen redirects stdout for test; no ownership transfer
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
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- fopen/fclose is POSIX C; no owning memory concern
    FILE* f = fopen(STDOUT_CAPTURE_PATH, "r");
    if (f != nullptr)
    {
        char buf[2048];
        size_t n = 0;
        // NOLINTNEXTLINE(clang-analyzer-unix.Stream) -- test helper; fread on EOF/error returns 0 and exits the loop cleanly
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        {
            content.append(buf, n);
        }
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- fclose is POSIX C; no owning memory concern
        fclose(f);
    }
    return content;
}

void RunWithInput(const char* input, BddTargetInteractiveSetHandler onSet)
{
    /* fmemopen takes a non-const void*; with mode "r" it never writes to
     * the buffer, so dropping const here is safe in practice. */
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast) -- see comment above; fmemopen mode "r" never writes
    FILE* in = fmemopen(const_cast<char*>(input), strlen(input), "r");
    struct SolidSyslogMessage message = {};
    /* These tests only exercise the set/switch/quit input paths, never `send`,
       so the SolidSyslog handle is never dereferenced — nullptr is safe. */
    BddTargetInteractive_Run(nullptr, &message, in, nullptr, onSet);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory) -- fclose is POSIX C; no owning memory concern
    fclose(in);
}

std::string RunCapturingStdout(const char* input, BddTargetInteractiveSetHandler onSet)
{
    StartStdoutCapture();
    RunWithInput(input, onSet);
    return EndStdoutCapture();
}
} // namespace

// clang-format off
TEST_GROUP(BddTargetInteractive)
{
    void setup() override
    {
        spy             = SetHandlerSpy{};
        spy.returnValue = true;
    }
};
// clang-format on

TEST(BddTargetInteractive, SetHandlerNotCalledWithQuitOnly)
{
    RunWithInput("quit\n", RecordSet);

    LONGS_EQUAL(0, spy.callCount);
}

TEST(BddTargetInteractive, SetCommandCallsHandlerOnce)
{
    RunWithInput("set hostname Foo\nquit\n", RecordSet);

    LONGS_EQUAL(1, spy.callCount);
}

TEST(BddTargetInteractive, SetCommandPassesNameToHandler)
{
    RunWithInput("set hostname Foo\nquit\n", RecordSet);

    STRCMP_EQUAL("hostname", spy.lastName.c_str());
}

TEST(BddTargetInteractive, SetCommandPassesValueToHandler)
{
    RunWithInput("set hostname Foo\nquit\n", RecordSet);

    STRCMP_EQUAL("Foo", spy.lastValue.c_str());
}

TEST(BddTargetInteractive, SetCommandWithoutValueGivesEmptyValue)
{
    RunWithInput("set hostname\nquit\n", RecordSet);

    STRCMP_EQUAL("hostname", spy.lastName.c_str());
    STRCMP_EQUAL("", spy.lastValue.c_str());
}

TEST(BddTargetInteractive, SetCommandWithEmbeddedSpacesPreservesValueAfterFirst)
{
    RunWithInput("set msg some text\nquit\n", RecordSet);

    STRCMP_EQUAL("msg", spy.lastName.c_str());
    STRCMP_EQUAL("some text", spy.lastValue.c_str());
}

TEST(BddTargetInteractive, NullSetHandlerSilentlyIgnoresSetLine)
{
    RunWithInput("set hostname Foo\nquit\n", nullptr);

    LONGS_EQUAL(0, spy.callCount);
}

TEST(BddTargetInteractive, SetCommandPrintsEchoOnHandlerSuccess)
{
    spy.returnValue = true;

    std::string captured = RunCapturingStdout("set hostname Foo\nquit\n", RecordSet);

    CHECK(captured.find("set hostname=Foo") != std::string::npos);
}

TEST(BddTargetInteractive, SetCommandPrintsInvalidOnHandlerFailure)
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
