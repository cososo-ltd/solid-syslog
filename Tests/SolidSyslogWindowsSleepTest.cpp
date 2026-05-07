#include "SolidSyslogWindowsSleep.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(SolidSyslogWindowsSleep)
{
};
// clang-format on

TEST(SolidSyslogWindowsSleep, ReturnsImmediatelyForZero)
{
    /* Sleep(0) yields the remainder of the thread's quantum and returns
       without blocking; the test pins that the wrapper does not crash. */
    SolidSyslogWindowsSleep(0);
}
