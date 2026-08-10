#include "SolidSyslogWindowsSleep.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(SolidSyslogWindows_Sleep)
{
};
// clang-format on

TEST(SolidSyslogWindows_Sleep, ReturnsImmediatelyForZero)
{
    /* Sleep(0) yields the remainder of the thread's quantum and returns
       without blocking; the test pins that the wrapper does not crash. */
    SolidSyslogWindows_Sleep(0);
}
