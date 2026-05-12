#include "CppUTest/TestHarness.h"
#include "SolidSyslogTunables.h"

/* When the build is configured with a user-tunables override (the
 * tunable-override-debug preset), CMake also injects the expected
 * value here. Asserting equality at compile time proves that the
 * SOLIDSYSLOG_USER_TUNABLES_FILE override actually flowed through to
 * the compiler. The default build leaves the macro undefined and this
 * assertion is compiled out. */
#ifdef SOLIDSYSLOG_TEST_EXPECTED_MAX_MESSAGE_SIZE
static_assert(SOLIDSYSLOG_MAX_MESSAGE_SIZE == SOLIDSYSLOG_TEST_EXPECTED_MAX_MESSAGE_SIZE, "SOLIDSYSLOG_USER_TUNABLES_FILE override did not reach the compiler");
#endif

TEST_GROUP(SolidSyslogTunables){};

TEST(SolidSyslogTunables, MaxMessageSizeIsReachableViaTunablesHeader)
{
    CHECK(SOLIDSYSLOG_MAX_MESSAGE_SIZE > 0);
}
