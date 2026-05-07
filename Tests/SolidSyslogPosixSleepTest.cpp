#include "SolidSyslogPosixSleep.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(SolidSyslogPosixSleep)
{
};
// clang-format on

TEST(SolidSyslogPosixSleep, ReturnsImmediatelyForZero)
{
    /* nanosleep with a zero-length duration is a defined no-op return; the
       test simply pins that the wrapper does not crash and returns under
       any vaguely reasonable wall-clock budget. Sub-millisecond completion
       is the expected behaviour on every supported POSIX platform. */
    SolidSyslogPosixSleep(0);
}
